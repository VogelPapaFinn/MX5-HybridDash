#include "Can.hpp"

// Project includes
#include "Events.hpp"

// espidf includes
#include "esp_event.h"
#include "esp_log.h"
#include "esp_twai.h"

/*
 *	Private constexpr
 */
constexpr auto TAG = "CanNode";

constexpr uint32_t CAN_SPEED = 1000000; // 1 Mbit/s
constexpr uint8_t CAN_QUEUE_DEPTH = 15;
constexpr uint16_t CAN_SEND_TIMEOUT_MS = 200;

/*
 *	Private Static Callback Functions
*/
static bool staticTransmittedFrameCb(twai_node_handle_t p_nodeHandle, const twai_tx_done_event_data_t* p_eventData,
									 void* p_userCtx)
{
	// Get the class context from the user arguments
	const auto can = static_cast<Can*>(p_userCtx);
	if (can == nullptr) {
		return false;
	}

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xQueueSendFromISR(*can->getFrameTransmittedQueue(), p_eventData->done_tx_frame, &xHigherPriorityTaskWoken);

	return xHigherPriorityTaskWoken;
}

static bool staticReceivedFrameCb(twai_node_handle_t p_nodeHandle, const twai_rx_done_event_data_t* p_eventData,
                                  void* p_userCtx)
{
	if (p_eventData == nullptr) {
		return false;
	}

	// Get the frame
	Can::Frame canFrame;
	twai_node_receive_from_isr(p_nodeHandle, &canFrame.twaiFrame);
	canFrame.fromTwaiFrame();

	// Queue it to the event loop
	esp_event_post(SYSTEM_EVENT_BASE, CAN_FRAME_RECEIVED, &canFrame, sizeof(canFrame), portMAX_DELAY);

	return false;
}

static bool staticStateChangedCb(twai_node_handle_t p_nodeHandle, const twai_state_change_event_data_t* p_eventData,
                                 void* p_userCtx)
{
	if (p_eventData == nullptr) {
		return false;
	}

	if (p_eventData->new_sta != TWAI_ERROR_BUS_OFF) {
		return false;
	}

	esp_rom_printf("State changed from %d to %d\n", p_eventData->old_sta, p_eventData->new_sta);
	// esp_rom_printf("An error occurred. Arbitration lost: %d \n Bit error: %d \n Form error: %d \n Stuff error: %d \n ACK error: %d \n val: %d", edata->err_flags.arb_lost, edata->err_flags.bit_err, edata->err_flags.form_err, edata->err_flags.stuff_err, edata->err_flags.ack_err, edata->err_flags.val);

	twai_node_recover(p_nodeHandle);

	return false;
}

/*
 *	Private Tasks
 */
static void canFrameTransmittedTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto can = static_cast<Can*>(p_param);

	twai_frame_t rxFrame;
	while (true) {
		if (xQueueReceive(*can->getFrameTransmittedQueue(), &rxFrame, portMAX_DELAY) != pdPASS) {
			continue;
		}

		can->transmittedFrameCb(rxFrame);
	}
}

/*
 *	Private Callback functions implementations
 */
void Can::transmittedFrameCb(const twai_frame_t frame)
{
	// Debug logging
	//esp_rom_printf("Transmitted frame with message id %d with buffer: ", p_eventData->done_tx_frame->header.id);
	//for (uint16_t i = 0; i < p_eventData->done_tx_frame->buffer_len; i++) {
	//	esp_rom_printf("%d ", p_eventData->done_tx_frame->buffer[i]);
	//}
	//esp_rom_printf("\n");

	if (xSemaphoreTake(pendingFramesMutex_, portMAX_DELAY) != pdTRUE) {
		return;
	}

	// Free memory
	for (uint32_t i = 0; i < pendingFrames_.size(); i++) {
		if (!pendingFrames_.at(i).get()->transmitting || pendingFrames_.at(i).get()->twaiFrame.header.id != frame.header.id) {
			continue;
		}

		auto const& pFrame = pendingFrames_.at(i).get()->twaiFrame;

		bool identical = true;

		// Header
		identical &= pFrame.header.id ==  frame.header.id;
		identical &= pFrame.header.dlc == frame.header.dlc;
		identical &= pFrame.header.brs == frame.header.brs;
		identical &= pFrame.header.esi == frame.header.esi;
		identical &= pFrame.header.fdf == frame.header.fdf;
		identical &= pFrame.header.ide == frame.header.ide;
		identical &= pFrame.header.rtr == frame.header.rtr;

		// Data
		identical &= pFrame.buffer_len == frame.buffer_len;
		if (!identical) { continue; }
		for (uint8_t j = 0; j < pFrame.buffer_len; j++) {
			identical &= pFrame.buffer[j] == frame.buffer[j];
		}

		if (!identical) { continue; }

		pendingFrames_.erase(pendingFrames_.begin() + i);

		// Unlock the mutex
		xSemaphoreGive(pendingFramesMutex_);

		return;
	}

	// Unlock the mutex
	xSemaphoreGive(pendingFramesMutex_);
}

/*
 *	Private Function implementations
 */
Can::Can(const gpio_num_t rxGpio, const gpio_num_t txGpio)
{
	rxGpio_ = rxGpio;
	txGpio_ = txGpio;

	nodeHandle_ = nullptr;

	nodeConfig_.io_cfg.rx = rxGpio_;
	nodeConfig_.io_cfg.tx = txGpio_;
	nodeConfig_.bit_timing.bitrate = CAN_SPEED;
	nodeConfig_.tx_queue_depth = CAN_QUEUE_DEPTH;
	nodeConfig_.intr_priority = ESP_INTR_FLAG_LEVEL1;

	nodeCallbacks_.on_tx_done = staticTransmittedFrameCb;
	nodeCallbacks_.on_rx_done = staticReceivedFrameCb;
	nodeCallbacks_.on_state_change = staticStateChangedCb;
	nodeCallbacks_.on_error = nullptr;

	pendingFramesMutex_ = xSemaphoreCreateMutex();
}

bool Can::initialize()
{
	if (rxGpio_ == GPIO_NUM_NC || txGpio_ == GPIO_NUM_NC) {
		return false;
	}

	if (twai_new_node_onchip(&nodeConfig_, &nodeHandle_) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize CAN node");
		return false;
	}

	if (twai_node_register_event_callbacks(nodeHandle_, &nodeCallbacks_, this) != ESP_OK) {
		// Cleanup
		twai_node_delete(nodeHandle_);

		ESP_LOGE(TAG, "Failed to register CAN node callbacks");
		return false;
	}

	/*
	 *	Create Task & Queue needed to handle the memory when a frame was transmitted
	 */
	frameTransmittedQueueHandle_ = xQueueCreate(CAN_QUEUE_DEPTH, sizeof(twai_frame_t));

	if (xTaskCreate(canFrameTransmittedTask, "CanFrameTransmittedTask", 2048, this, 2, &frameTransmittedTaskHandle_) != pdPASS) {
		ESP_LOGE(TAG, "Failed to create can frame transmitted task");
		esp_restart();
	}

	ESP_LOGI(TAG, "Initialized CAN node");

	return true;
}

void Can::deinitialize() const
{
	if (nodeHandle_ == nullptr) {
		return;
	}

	// There is no function for unregistering the callbacks, so simply delete the node
	twai_node_delete(nodeHandle_);

	ESP_LOGI(TAG, "Deinitialized CAN node");
}

bool Can::enable()
{
	if (nodeHandle_ == nullptr || enabled_) {
		return false;
	}

	enabled_ = twai_node_enable(nodeHandle_) == ESP_OK;
	if (!enabled_) {
		ESP_LOGE(TAG, "Failed to enable CAN node");
	}
	else {
		ESP_LOGI(TAG, "Enabled CAN node");
	}

	return enabled_;
}

void Can::queueFrame(Frame& canFrame)
{
	if (!enabled_) {
		return;
	}

	canFrame.transmitting = true;

	// ESP_LOGI(TAG, "Transmitting Frame %s", canFrame.toString().c_str());

	// Acquire the mutex lock
	if (xSemaphoreTake(pendingFramesMutex_, portMAX_DELAY) != pdTRUE) {
		return;
	}

	// Track the new frame
	pendingFrames_.push_back(std::make_shared<Frame>(canFrame));
	const auto trackedFrame = pendingFrames_.back().get();
	trackedFrame->buildTwaiFrame();

	// Unlock the mutex
	xSemaphoreGive(pendingFramesMutex_);

	if (twai_node_transmit(nodeHandle_, &trackedFrame->twaiFrame, CAN_SEND_TIMEOUT_MS) != ESP_OK) {
		ESP_LOGW(TAG, "Failed to queue CAN frame");
	}
}

QueueHandle_t* Can::getFrameTransmittedQueue()
{
	return &frameTransmittedQueueHandle_;
}
