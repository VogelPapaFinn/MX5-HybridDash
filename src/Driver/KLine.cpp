#include "Driver/KLine.hpp"

// C++ includes
#include <bits/stdc++.h>
#include <string>

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

/*
 *	Private Static Helper Functions
 */
static void staticReceivedUartMessage(KLine* p_kline, uart_event_t& event)
{
	/*
	 *	Pull the data from the UART port
	 */
	KLine::KLineMessage message;
	message.rawMessage.resize(event.size);

	// Try to read the data
	const int bytesRead = uart_read_bytes(UART_PORT, message.rawMessage.data(), event.size, portMAX_DELAY);
	if (bytesRead <= 0) {
		return;
	}

	// Fill the struct
	message.fromRawMessage();

	/*
	 *	Check if it's a message we sent, because we can ignore them
	 */
	bool ignore = false;

	// Acquire the mutex, to prevent racing conditions
	if (xSemaphoreTake(p_kline->getLastMessageMutex(), portMAX_DELAY) != pdFALSE) {
		auto& lastMessages = p_kline->getLastMessagesSent();
		for (const auto& m : lastMessages) {
			/*
			 *	This was NOT a message that we sent, so skip it
			 */
			if (message != m) {
				continue;
			}

			/*
			 *	This was a message that we sent
			 */
			ignore = true;
			lastMessages.remove(m);
			break;
		}

		// Return the semaphore
		xSemaphoreGive(p_kline->getLastMessageMutex());
	}

	/*
	 *	Ignore the message if necessary
	 */
	if (ignore) {
		// ESP_LOGI(TAG, "Ignored message with %d bytes", event.size);
		return;
	}

	/*
	 *	Debug logging
	 */
	// esp_rom_printf("event.size: %d, bytesRead: %d\n", event.size, bytesRead);
	// for (auto& c : message.rawMessage) {
	// 	esp_rom_printf("Received: %d\n", c);
	// }

	/*
	 *	Is it the read ECU ID message?
	 */
	constexpr auto NEEDED_SID = SID_READ_FLASH + SID_RESPONSE;
	const auto neededPid =
		std::vector<uint8_t>{static_cast<uint8_t>(ECU_ID_ADDR >> 8) & 0xFF, static_cast<uint8_t>(ECU_ID_ADDR) & 0xFF};

	if (NEEDED_SID == message.sid && neededPid == message.pid) {
		ESP_LOGI(TAG, "Received ECU ID: %c%c%c%c", message.data[0], message.data[1], message.data[2], message.data[3]);
		return;
	}

	/*
	 *	Queue event to the event loop
	 */
	// esp_event_post(SYSTEM_EVENT_BASE, KLINE_MSG_RECEIVED, &message, sizeof(message), portMAX_DELAY);

	/*
	 *	Track new sensor value, if it is one
	 *	TODO: Move to the Sensor class(es)
	 */
	if (message.sid == SID_RDBI + SID_RESPONSE) {
		// Get the PID
		const uint16_t pid = (message.pid.at(0) << 8) + message.pid.at(1);

		// Get the data
		uint16_t data = 0;
		if (message.data.size() >= 2) {
			data = (message.pid.at(0) << 8) + message.pid.at(1);
		}
		else {
			data = message.pid.at(0);
		}

		// Save the new value
		if (ECU_SENSORS.contains(pid)) {
			ECU_SENSORS[pid].rawValue = data;

			// ESP_LOGI(TAG, "Updated Sensor with name %s to %d", ECU_SENSORS[pid].name.c_str(), data);
		}
	}
}

/*
 *	Private Static Tasks
 */
static void staticUartRxTask(void* p_param)
{
	/*
	 *	Preparations
	 */
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	// Get the reference to the KLine instance
	KLine* kline = static_cast<KLine*>(p_param);
	const auto queue = kline->getQueue();

	// Contains the queue event
	uart_event_t event;

	/*
	 *	Handle new queue events
	 */
	while (true) {
		// Pull the new event
		if (xQueueReceive(queue, &event, portMAX_DELAY) == pdFALSE) {
			continue;
		}

		// Act depending on the event type
		switch (event.type) {
			/*
			 *	 Received Data
			 */
			case UART_DATA:
				staticReceivedUartMessage(kline, event);
				break;

			/*
			 *	Buffer Overflow
			 */
			case UART_FIFO_OVF:
			case UART_BUFFER_FULL:
				ESP_LOGW(TAG, "UART Buffer Overlow. Flushing");
				uart_flush_input(UART_PORT);
				xQueueReset(queue);
				break;

			default:
				break;
		}
	}
}

/*
 *	Public Function Implementations
 */
KLine::KLine()
{
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
	if (xTaskCreate(staticUartRxTask, "KLineRxTask", 2048 * 2, this, 2, &rxTaskHandle_) != pdPASS) {
		ESP_LOGE(TAG, "Failed to create RX Task");
		return;
	}

	// Create the mutex for the lastMessage_
	lastMessageMutex_ = xSemaphoreCreateMutex();

	initialized_ = true;
}

KLine::~KLine() { vTaskDelete(rxTaskHandle_); }

void KLine::readEcuId()
{
	ESP_LOGI(TAG, "Reading ECU ID");

	if (!initialized_) {
		return;
	}

	// Build the message
	KLineMessage message{.length = 0x7,
						 .command = 0x4,
						 .sender = ADDR_ECU,
						 .receiver = ADDR_PCB,
						 .sid = SID_READ_FLASH,
						 .pid = {static_cast<uint8_t>(ECU_ID_ADDR >> 16) & 0xFF,
								 static_cast<uint8_t>(ECU_ID_ADDR >> 8) & 0xFF,
								 static_cast<uint8_t>(ECU_ID_ADDR) & 0xFF}};
	message.calculateChecksum();

	// Send it
	send(message);
}

void KLine::readPid(const uint16_t pid)
{
	if (!initialized_) {
		return;
	}

	// Build the message
	KLineMessage message{.length = 0x6,
						 .command = 0x4,
						 .sender = ADDR_ECU,
						 .receiver = ADDR_PCB,
						 .sid = SID_RDBI,
						 .pid = {static_cast<uint8_t>(pid >> 8), static_cast<uint8_t>(pid & 0xFF)}};
	message.calculateChecksum();

	// Send it
	send(message);
}

QueueHandle_t KLine::getQueue() const { return uartQueueHandle_; }

SemaphoreHandle_t KLine::getLastMessageMutex() const { return lastMessageMutex_; }

std::list<KLine::KLineMessage>& KLine::getLastMessagesSent() { return lastMessagesSent_; }

/*
 *	Private Function Implementations
 */
void KLine::send(KLineMessage& message)
{
	// Save it as the latest message that was sent
	if (xSemaphoreTake(lastMessageMutex_, portMAX_DELAY) != pdFALSE) {
		lastMessagesSent_.push_back(message);

		xSemaphoreGive(lastMessageMutex_);
	}

	// Convert the struct to an array
	message.toRawMessage();

	/*
	 * Debug logging
	 */
	// for (auto& c : message.rawMessage) {
	//	esp_rom_printf("Sending: %d\n", c);
	// }

	/*
	 *	Send the message via UART to the ECU
	 */
	uart_flush_input(UART_PORT);
	const auto bytesWritten = uart_write_bytes(UART_PORT, message.rawMessage.data(), message.rawMessage.size());
	if (bytesWritten < message.rawMessage.size()) {
		ESP_LOGW(TAG, "Failed to send full message. Only send %d bytes", bytesWritten);
	}
}
