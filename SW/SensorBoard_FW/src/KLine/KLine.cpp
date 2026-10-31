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

// KLine timings (ISO 9141)
constexpr uint32_t WAIT_FOR_BYTE_TIMEOUT_MS = 20;
constexpr uint32_t WAIT_FOR_ANSWER_TIMEOUT_MS = 40;
constexpr uint32_t DELAY_BETWEEN_TX_MESSAGES_MS = 25;

constexpr uint32_t KEEP_ALIVE_TIME_MS = 4000;
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

	requestQueue_ = xQueueCreate(10, sizeof(Request*));
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

bool KLine::init()
{
	if (initialized_) {
		return false;
	}

	// baud5Init();

	if (xTaskCreate(klineTask, "KLineTask", 4096, this, 2, &taskHandle_) != pdPASS) {
		ESP_LOGE(TAG, "Failed to create KLine Task");
		return false;
	}

	initialized_ = true;

	return true;
}

bool KLine::send(const KLineMessage& message, std::function<void(KLineMessage&)> rxCb)
{
	if (!initialized_) {
		return false;
	}

	// ESP_LOGI(TAG, "Sending request: %s", message.toString().c_str());

	auto request = new Request{message, std::move(rxCb)};
	if (xQueueSend(requestQueue_, &request, portMAX_DELAY) != pdTRUE) {
		delete (request);
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
	if (!setUartGpios()) {
		ESP_LOGE(TAG, "Failed to set the GPIOs for UART");
		return false;
	}

	// Move every byte immediately from the HW FIFO to the RX buffer. Otherwise the driver waits
	// until the FIFO threshold or the RX timeout (~10 byte times) is reached, which breaks the byte timeouts
	uart_set_rx_full_threshold(UART_PORT, 1);
	uart_set_rx_timeout(UART_PORT, 1);

	return true;
}

bool KLine::setUartGpios()
{
	const auto result = uart_set_pin(UART_PORT, GPIO_TX, GPIO_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) == ESP_OK;

	uart_flush_input(UART_PORT);

	return result;
}

bool KLine::baud5Init()
{
	ESP_LOGI(TAG, "Starting 5-baud init");

	send33Init();

	ESP_LOGI(TAG, "Waiting for 0x55");

	// Read 0x55
	uint8_t firstResponse = 0;
	const auto tmp = uart_read_bytes(UART_PORT, &firstResponse, 1, pdMS_TO_TICKS(300));
	if (tmp != 1 || firstResponse != 0x55) {
		ESP_LOGI(TAG, "%d", tmp);
		ESP_LOGE(TAG, "Didn't receive 0x55 answer from ECU during init!");
		return false;
	}

	ESP_LOGI(TAG, "Waiting for 0x808");

	// Read 0x0808
	uint8_t secondResponse[2] = {};
	if (uart_read_bytes(UART_PORT, &secondResponse, 2, pdMS_TO_TICKS(WAIT_FOR_ANSWER_TIMEOUT_MS)) != 2 ||
		secondResponse[0] != 0x08 || secondResponse[1] != 0x08) {
		ESP_LOGE(TAG, "Didn't receive 0x0808 answer from ECU during init!");
		return false;
	}

	// Transmit 0xF7
	vTaskDelay(pdMS_TO_TICKS(DELAY_BETWEEN_TX_MESSAGES_MS));
	if (!transmit({0xF7})) {
		ESP_LOGE(TAG, "Failed to send 0xF7 to ECU during init!");
		return false;
	}
	receiveEcho({0xF7});

	// Read 0xCC
	uint8_t thirdResponse = 0;
	if (uart_read_bytes(UART_PORT, &thirdResponse, 1, pdMS_TO_TICKS(WAIT_FOR_ANSWER_TIMEOUT_MS)) != 1 ||
		thirdResponse != 0xCC) {
		ESP_LOGE(TAG, "Didn't receive 0xCC answer from ECU during init!");
		return false;
	}

	return true;
}

void KLine::send33Init()
{
	// Reclaim GPIO
	gpio_set_level(GPIO_TX, 1);
	gpio_set_direction(GPIO_TX, GPIO_MODE_OUTPUT);

	// Send start bit
	vTaskDelay(pdMS_TO_TICKS(300)); // Needed before the start bit
	gpio_set_level(GPIO_TX, 0);
	vTaskDelay(pdMS_TO_TICKS(1000 / 5)); // Wait 200ms = 5-baud

	ESP_LOGI(TAG, "Transmitting 0x33");

	const uint8_t initVal = 0x33;
	TickType_t ticks = xTaskGetTickCount();
	for (uint8_t i = 0; i < 8; i++) {
		gpio_set_level(GPIO_TX, initVal & (0b00000001 << i));
		vTaskDelayUntil(&ticks, pdMS_TO_TICKS(1000 / 5)); // Wait 200ms = 5-baud
	}

	ESP_LOGI(TAG, "Finished transmitting 0x33");
	vTaskDelayUntil(&ticks, pdMS_TO_TICKS(300));

	// Send stop bit
	gpio_set_level(GPIO_TX, 1);

	// Return GPIOs
	setUartGpios();
}

void KLine::klineTask(void* p_param)
{
	auto kline = static_cast<KLine*>(p_param);

	Request* request = nullptr;
	while (true) {
		if (xQueueReceive(kline->requestQueue_, &request, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		KLineMessage response = kline->process(*request);

		if (response != KLineMessage() && request->callback) {
			request->callback(response);
		}

		delete (request);
		request = nullptr;

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
	if (uart_write_bytes(UART_PORT, raw.data(), raw.size()) != raw.size()) {
		return false;
	}

	if (uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(TX_BYTE_TIMEOUT_MS)) != ESP_OK) {
		return false;
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

	// ESP_LOGD(TAG, "Received possible echo");

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

	// ESP_LOGI(TAG, "Received response: %s", response.toString().c_str());

	return response;
}

// TODO:
// - 5 Baud Init
