#include "KLine/KLine.hpp"

// C++ includes
#include <utility>

// espidf include
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

/*
 *	constexpr
 */
constexpr auto TAG = "KLine";

// UART stuff
constexpr uart_port_t UART_PORT = UART_NUM_2;
constexpr uint16_t BUFFER_SIZE_RX = 256; // Has to be bigger than the HW FIFO (128 bytes)
constexpr uint16_t BUFFER_SIZE_TX = 0; // No TX buffer, uart_write_bytes blocks until the data is in the HW FIFO
constexpr uint32_t UART_BAUD_RATE = 10400;
constexpr uart_config_t UART_CONFIG = {
	.baud_rate = UART_BAUD_RATE,
	.data_bits = UART_DATA_8_BITS,
	.parity = UART_PARITY_DISABLE,
	.stop_bits = UART_STOP_BITS_1,
	.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	.rx_flow_ctrl_thresh = 122,
};

// GPIO stuff
constexpr gpio_num_t GPIO_RX = GPIO_NUM_12;
constexpr gpio_num_t GPIO_TX = GPIO_NUM_11;

// KLine timings (ISO 14230)
constexpr uint32_t WAIT_FOR_BYTE_TIMEOUT_MS = 20;
constexpr uint32_t WAIT_FOR_ANSWER_TIMEOUT_MS = 50;
constexpr uint32_t DELAY_BETWEEN_TX_MESSAGES_MS = 55;
constexpr uint32_t DELAY_BETWEEN_TX_BYTES_MS = 5000;

constexpr uint32_t ECHO_TIMEOUT_MS = 20;
constexpr uint32_t TX_BYTE_TIMEOUT_MS = 10;

/*
 *	Public Function Implementations
 */
KLine::KLine()
{
	if (!setupUart()) {
		ESP_LOGE(TAG, "Failed to setup UART");
		return;
	}

	// Move every byte immediately from the HW FIFO to the RX buffer. Otherwise the driver waits
	// until the FIFO threshold or the RX timeout (~10 byte times) is reached, which breaks the byte timeouts
	uart_set_rx_full_threshold(UART_PORT, 1);
	uart_set_rx_timeout(UART_PORT, 1);

	requestQueue_ = xQueueCreate(10, sizeof(Request*));

	if (xTaskCreate(klineTask, "KLineTask", 4096, this, 2, &taskHandle_) != pdPASS) {
		ESP_LOGE(TAG, "Failed to create KLine Task");
		return;
	}

	initialized_ = true;
}

KLine::~KLine()
{
	if (taskHandle_ != nullptr) {
		vTaskDelete(taskHandle_);
	}

	if (requestQueue_ != nullptr) {
		vQueueDelete(requestQueue_);
	}

	uart_driver_delete(UART_PORT);
}

bool KLine::send(const KLineMessage& message, std::function<void(KLineMessage&)> rxCb)
{
	if (!initialized_) {
		return false;
	}

	Request request{message, std::move(rxCb)};
	if (xQueueSend(requestQueue_, &request, portMAX_DELAY) != pdTRUE) {
		return false;
	}

	return true;
}

/*
 *	Private Function Implementations
 */
bool KLine::setupUart()
{
	// Initialize UART driver (no event queue, we read synchronously)
	if (uart_driver_install(UART_PORT, BUFFER_SIZE_RX, BUFFER_SIZE_TX, 0, nullptr, 0) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to install UART driver");
		return false;
	}

	// Initialize UART configuration
	if (uart_param_config(UART_PORT, &UART_CONFIG) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to parameterize UART");
		return false;
	}

	// Set GPIO pins for UART
	if (uart_set_pin(UART_PORT, GPIO_TX, GPIO_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to set the GPIOs for UART");
		return false;
	}

	return true;
}

void KLine::klineTask(void* p_param)
{
	auto kline = static_cast<KLine*>(p_param);

	Request request;
	while (true) {
		if (xQueueReceive(kline->requestQueue_, &request, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		KLineMessage response = kline->process(request);

		if (response != KLineMessage() && request.callback) {
			request.callback(response);
		}

		// Wait before sending the next request
		vTaskDelay(pdMS_TO_TICKS(DELAY_BETWEEN_TX_MESSAGES_MS));
	}
}

KLineMessage KLine::process(Request& request)
{
	const auto rawRequest = request.message.getRaw();

	uart_flush_input(UART_PORT);

	if (!transmit(rawRequest)) {
		ESP_LOGW(TAG, "Failed to send %s", request.message.toString().c_str());
		return KLineMessage();
	}

	if (!receiveEcho(rawRequest)) {
		ESP_LOGW(TAG, "Echo mismatch for %s", request.message.toString().c_str());
		return KLineMessage();
	}

	return receiveResponse();
}

bool KLine::transmit(const std::vector<uint8_t>& raw)
{
	for (const auto b : raw) {
		if (uart_write_bytes(UART_PORT, &b, 1) != 1) {
			return false;
		}

		if (uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(TX_BYTE_TIMEOUT_MS) != ESP_OK)) {
			return false;
		}

		// Wait between each byte (needed for the ECU to understand and process)
		if (b != raw.back()) {
			vTaskDelay(pdMS_TO_TICKS(DELAY_BETWEEN_TX_BYTES_MS));
		}
	}

	return true;
}

bool KLine::receiveEcho(const std::vector<uint8_t>& rawRequest)
{
	std::vector<uint8_t> echo(rawRequest.size());

	const int bytesRead = uart_read_bytes(UART_PORT, echo.data(), echo.size(), pdMS_TO_TICKS(ECHO_TIMEOUT_MS));
	if (bytesRead != rawRequest.size()) {
		return false;
	}

	return echo == rawRequest;
}

KLineMessage KLine::receiveResponse()
{
	std::vector<uint8_t> raw(1);

	// First byte contains the length
	if (uart_read_bytes(UART_PORT, raw.data(), 1, pdMS_TO_TICKS(WAIT_FOR_ANSWER_TIMEOUT_MS)) != 1) {
		return KLineMessage();
	}

	// Check if length is valid
	const uint8_t length = (raw[0] >> 4) & 0b1111;
	if (length < 4) {
		ESP_LOGW(TAG, "Response with invalid length %d", length);
		return KLineMessage();
	}

	// Read the rest of the response
	raw.resize(length + 1);
	for (uint8_t i = 1; i <= length; i++) {
		if (uart_read_bytes(UART_PORT, &raw[i], 1, pdMS_TO_TICKS(WAIT_FOR_BYTE_TIMEOUT_MS)) != 1) {
			ESP_LOGW(TAG, "Incomplete response, got %d of %d bytes", i, length + 1);
			return KLineMessage();
		}
	}

	KLineMessage response;
	if (!response.fromRaw(raw)) {
		return KLineMessage();
	}

	return response;
}
