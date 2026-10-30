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

// KLine stuff
constexpr uint8_t COMMAND = 0x04;
constexpr uint8_t ADDR_ECU = 0x10;
constexpr uint8_t ADDR_PCB = 0xF5;
constexpr uint8_t READ_SENSOR = 0x17;
constexpr uint32_t ECU_ID_ADDR = 0x010006;

// KLine timings (ISO 14230)
constexpr uint32_t P1_MAX_MS = 20; // Max. inter byte time of the ECU response
constexpr uint32_t P2_MAX_MS = 50; // Max. time between end of request and start of response
constexpr uint32_t P3_MIN_MS = 55; // Min. time between end of response and next request
constexpr uint32_t P4_MIN_US = 5000; // Min. inter byte time of our request
constexpr uint32_t ECHO_TIMEOUT_MS = 20;
constexpr uint32_t TX_BYTE_TIMEOUT_MS = 10;

// Task stuff
constexpr uint8_t REQUEST_QUEUE_SIZE = 10;
constexpr uint32_t TASK_STACK_SIZE = 4096;
constexpr UBaseType_t TASK_PRIORITY = 2;

/*
 *	Private Static Helper Functions
 */

// pdMS_TO_TICKS rounds down (with 100 Hz, 5 ms -> 0 ticks). This guarantees to wait at least ms
static TickType_t atLeastMs(const uint32_t ms) { return pdMS_TO_TICKS(ms) + 1; }

/*
 *	Public Function Implementations
 */
KLine::KLine()
{
	// Initialize UART driver (no event queue, we read synchronously)
	if (uart_driver_install(UART_PORT, BUFFER_SIZE_RX, BUFFER_SIZE_TX, 0, nullptr, 0) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to install UART driver");
		return;
	}

	// Initialize UART configuration
	if (uart_param_config(UART_PORT, &UART_CONFIG) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to parameterize UART");
		return;
	}

	// Set GPIO pins for UART
	if (uart_set_pin(UART_PORT, GPIO_TX, GPIO_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to set the GPIOs for UART");
		return;
	}

	// Move every byte immediately from the HW FIFO to the RX buffer. Otherwise the driver waits
	// until the FIFO threshold or the RX timeout (~10 byte times) is reached, which breaks the byte timeouts
	uart_set_rx_full_threshold(UART_PORT, 1);
	uart_set_rx_timeout(UART_PORT, 1);

	// Create the request queue
	requestQueue_ = xQueueCreate(REQUEST_QUEUE_SIZE, sizeof(Request*));
	if (requestQueue_ == nullptr) {
		ESP_LOGE(TAG, "Failed to create the request queue");
		return;
	}

	// Create the task
	if (xTaskCreate(klineTask, "KLineTask", TASK_STACK_SIZE, this, TASK_PRIORITY, &taskHandle_) != pdPASS) {
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
		Request* request = nullptr;
		while (xQueueReceive(requestQueue_, &request, 0) == pdTRUE) {
			delete request;
		}
		vQueueDelete(requestQueue_);
	}

	uart_driver_delete(UART_PORT);
}

bool KLine::send(const KLineMessage& message, ResponseCallback rxCb)
{
	if (!initialized_) {
		return false;
	}

	auto request = new Request{message, std::move(rxCb)};
	if (xQueueSend(requestQueue_, &request, portMAX_DELAY) != pdTRUE) {
		delete request;
		return false;
	}

	return true;
}

/*
 *	Private Function Implementations
 */
void KLine::klineTask(void* p_param)
{
	auto kline = static_cast<KLine*>(p_param);

	Request* request = nullptr;
	while (true) {
		if (xQueueReceive(kline->requestQueue_, &request, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		KLineMessage response;
		const Result result = kline->process(*request, response);

		if (request->callback) {
			request->callback(result, response);
		}
		delete request;

		// Wait before sending the next request
		vTaskDelay(atLeastMs(P3_MIN_MS));
	}
}

KLine::Result KLine::process(Request& request, KLineMessage& response)
{
	const auto raw = request.message.getRaw();

	// Discard everything that's left from before
	uart_flush_input(UART_PORT);

	if (!transmit(raw)) {
		ESP_LOGW(TAG, "Failed to send %s", request.message.toString().c_str());
		return Result::TxFailed;
	}

	// KLine is a single wire, so we receive everything we send
	if (!receiveEcho(raw)) {
		ESP_LOGW(TAG, "Echo mismatch for %s", request.message.toString().c_str());
		return Result::EchoMismatch;
	}

	return receiveResponse(response);
}

bool KLine::transmit(const std::vector<uint8_t>& raw)
{
	for (size_t i = 0; i < raw.size(); i++) {
		if (uart_write_bytes(UART_PORT, &raw[i], 1) != 1) {
			return false;
		}

		if (uart_wait_tx_done(UART_PORT, atLeastMs(TX_BYTE_TIMEOUT_MS)) != ESP_OK) {
			return false;
		}

		// P4 between the bytes, not after the last one. Busy wait, because the tick is too coarse
		if (i + 1 < raw.size()) {
			esp_rom_delay_us(P4_MIN_US);
		}
	}

	return true;
}

bool KLine::receiveEcho(const std::vector<uint8_t>& raw)
{
	std::vector<uint8_t> echo(raw.size());

	const int bytesRead = uart_read_bytes(UART_PORT, echo.data(), echo.size(), atLeastMs(ECHO_TIMEOUT_MS));
	if (bytesRead != static_cast<int>(raw.size())) {
		return false;
	}

	return echo == raw;
}

KLine::Result KLine::receiveResponse(KLineMessage& response)
{
	std::vector<uint8_t> raw(1);

	// First byte: length (upper nibble) + command (lower nibble)
	if (uart_read_bytes(UART_PORT, raw.data(), 1, atLeastMs(P2_MAX_MS)) != 1) {
		return Result::NoResponse;
	}

	// Length counts sender, target, SID, payload and checksum
	const uint8_t length = (raw[0] >> 4) & 0b1111;
	if (length < 4) {
		ESP_LOGW(TAG, "Response with invalid length %d", length);
		return Result::InvalidResponse;
	}

	// Read the remaining bytes, each one has to arrive within P1max
	raw.resize(length + 1);
	for (uint8_t i = 1; i <= length; i++) {
		if (uart_read_bytes(UART_PORT, &raw[i], 1, atLeastMs(P1_MAX_MS)) != 1) {
			ESP_LOGW(TAG, "Incomplete response, got %d of %d bytes", i, length + 1);
			return Result::InvalidResponse;
		}
	}

	// raw is complete now, so fromRaw can't run out of bounds
	if (!response.fromRaw(raw)) {
		return Result::InvalidResponse;
	}

	return Result::Ok;
}
