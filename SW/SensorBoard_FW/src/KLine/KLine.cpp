#include "KLine/KLine.hpp"

// C++ includes
#include <bits/stdc++.h>
#include <string>
#include <utility>

// Project includes
#include "Driver/EcuSensors.hpp"

// espidf include
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_event.h"
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "KLine";

// UART stuff
constexpr uart_port_t UART_PORT = UART_NUM_2;
constexpr uint16_t BUFFER_SIZE_RX = 2048;
constexpr uint16_t BUFFER_SIZE_TX = 2048;
constexpr uint8_t UART_QUEUE_SIZE = 10;
constexpr uint16_t UART_BAUD_RATE = 10400;
constexpr uart_config_t UART_CONFIG = {
	.baud_rate = 10400,
	.data_bits = UART_DATA_8_BITS,
	.parity = UART_PARITY_DISABLE,
	.stop_bits = UART_STOP_BITS_1,
	.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	.rx_flow_ctrl_thresh = 122,
};

// GPIO stuff
constexpr gpio_num_t GPIO_RX = GPIO_NUM_12;
constexpr gpio_num_t GPIO_TX = GPIO_NUM_11;

// KLine stuff
constexpr uint8_t COMMAND = 0x04;
constexpr uint8_t ADDR_ECU = 0x10;
constexpr uint8_t ADDR_PCB = 0xF5;
constexpr uint8_t READ_SENSOR = 0x17;
constexpr uint32_t ECU_ID_ADDR = 0x010006;

constexpr uint8_t WAIT_FOR_ANSWER_TIMEOUT_MS = 50;
constexpr uint8_t WAIT_FOR_BYTE_TIMEOUT_MS = 20;
constexpr uint8_t DELAY_BETWEEN_RX_TX_MS = 60;

constexpr uint8_t DELAY_BETWEEN_TX_BYTES_MS = 5;
constexpr uint8_t DELAY_BETWEEN_TX_MESSAGES_MS = 55;

/*
 *	Public Function Implementations
 */
KLine::KLine()
{
	waitForAnswerTimeout_ =
		xTimerCreate("WaitForAnswerTimeout", pdMS_TO_TICKS(WAIT_FOR_ANSWER_TIMEOUT_MS), false, this, noAnswerReceived);
	waitForByteTimeout_ =
		xTimerCreate("WaitForByteTimeout", pdMS_TO_TICKS(WAIT_FOR_BYTE_TIMEOUT_MS), false, this, noAnswerReceived);

	pendingMessagesQueue_ = xQueueCreate(10, sizeof(KLineMessage));

	// Initialize UART driver
	if (uart_driver_install(UART_PORT, BUFFER_SIZE_RX, BUFFER_SIZE_TX, UART_QUEUE_SIZE, &uartQueueHandle_, 0) !=
		ESP_OK) {
		ESP_LOGE(TAG, "Failed to install UART driver");
		return;
	}

	// Initialize UART configuration
	if (uart_param_config(UART_PORT, &UART_CONFIG)) {
		ESP_LOGE(TAG, "Failed to parameterize UART");
		return;
	}

	// Set GPIO pins for UART
	if (uart_set_pin(UART_NUM_2, GPIO_TX, GPIO_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)) {
		ESP_LOGE(TAG, "Failed to set the GPIOs for UART");
		return;
	}

	// Create RX task
	if (xTaskCreate(uartRxTask, "KLineRxTask", 2048 * 2, this, 2, &rxTaskHandle_) != pdPASS) {
		ESP_LOGE(TAG, "Failed to create RX Task");
		return;
	}

	initialized_ = true;
}

KLine::~KLine() { vTaskDelete(rxTaskHandle_); }

bool KLine::send(KLineMessage& message, std::function<void()>* p_rxCb)
{
	if (observers_.contains(message)) {
		return false;
	}

	xQueueSend(pendingMessagesQueue_, &message, portMAX_DELAY);
	observers_[message] = p_rxCb;

	return true;
}

/*
 *	Private Function Implementations
 */
void KLine::uartTxTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto kline = static_cast<KLine*>(p_param);

	KLineMessage message;
	while (true) {
		if (xQueueReceive(kline->pendingMessagesQueue_, &message, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		kline->sendEachByte(message);

		// Wait until echo and new message were received
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Wait before sending again
		vTaskDelay(pdMS_TO_TICKS(DELAY_BETWEEN_TX_MESSAGES_MS));
	}
}

void KLine::sendEachByte(KLineMessage& message)
{
	const auto rawMessage = message.getRaw();

	echo_ = message;

	for (const auto& b : rawMessage) {
		uart_write_bytes(UART_PORT, &b, 1);
		uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(DELAY_BETWEEN_TX_BYTES_MS));
		vTaskDelay(pdMS_TO_TICKS(DELAY_BETWEEN_TX_BYTES_MS));
	}
}

void KLine::uartRxTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	KLine* kline = static_cast<KLine*>(p_param);
	uart_event_t event;

	while (true) {
		if (xQueueReceive(kline->uartQueueHandle_, &event, portMAX_DELAY) == pdFALSE) {
			continue;
		}

		switch (event.type) {
			case UART_DATA: {
				xTimerStop(kline->waitForAnswerTimeout_, 0);

				// Pull received data
				std::vector<uint8_t> fragment(event.size);
				uart_read_bytes(UART_PORT, fragment.data(), event.size, portMAX_DELAY);

				// Add it to the other fragments
				for (const auto& b : fragment) {
					kline->rxMessageFragments_.push_back(b);
				}

				// Check if it was the end of a message
				if (!kline->isDataAMessage(kline->rxMessageFragments_)) {
					xTimerStart(kline->waitForByteTimeout_, 0);
					continue;
				}
				xTimerStop(kline->waitForByteTimeout_, 0);

				KLineMessage m;
				m.fromRaw(kline->rxMessageFragments_);

				// Ignore echo
				if (kline->echo_ == m) {
					kline->echo_ = KLineMessage();
					kline->rxMessageFragments_.clear();

					xTimerStart(kline->waitForAnswerTimeout_, 0);

					return;
				}

				// TODO
				// notify decoder task
				kline->rxMessageFragments_.clear();
			} break;

			case UART_FIFO_OVF:
			case UART_BUFFER_FULL: {
				ESP_LOGW(TAG, "UART Buffer Overlow. Flushing");

				uart_flush_input(UART_PORT);

				xQueueReset(kline->uartQueueHandle_);
				kline->rxMessageFragments_.clear();
			} break;

			default:
				break;
		}
	}
}

bool KLine::isDataAMessage(std::vector<uint8_t>& data)
{
	KLineMessage m;
	return m.fromRaw(data);
}

void KLine::noAnswerReceived(TimerHandle_t p_timer)
{
	auto kline = static_cast<KLine*>(pvTimerGetTimerID(p_timer));

	kline->echo_ = KLineMessage();
	kline->rxMessageFragments_.clear();

	xTaskNotifyGive(kline->txTaskHandle_);
}

// TODO:
// - Decoder Task, der die korrekte Message entgegen nimmt, den txTask notified und den observer benachrichtigt
// - Init Fehlt (welcher den txTaskHandle und rxTaskHandle setzt)
// - Code überarbeiten und besser strukturieren, benennen etc
