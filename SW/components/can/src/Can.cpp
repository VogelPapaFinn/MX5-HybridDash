#include "../Can.hpp"

// espidf includes
#include "esp_log.h"
#include "esp_twai.h"

/*
 *	Private constexpr
 */
constexpr auto TAG = "CanNode";

constexpr uint32_t CAN_SPEED = 1000000; // 1 Mbit/s
constexpr uint8_t  CAN_QUEUE_DEPTH = 15;
constexpr uint16_t CAN_SEND_TIMEOUT_MS = 200;

/*
 *	Private Static Callback Functions
 */
static bool staticReceivedFrameCb(twai_node_handle_t nodeHandle, const twai_rx_done_event_data_t* p_eventData, void* p_userCtx)
{
	// Get the class context from the user arguments
	auto canNode = static_cast<Can*>(p_userCtx);
	if (canNode == nullptr) {
		return false;
	}

	return canNode->receivedFrameCb(nodeHandle, p_eventData, nullptr);
}

static bool staticTransmittedFrameCb(twai_node_handle_t nodeHandle, const twai_tx_done_event_data_t* p_eventData, void* p_userCtx)
{
	// Get the class context from the user arguments
	auto canNode = static_cast<Can*>(p_userCtx);
	if (canNode == nullptr) {
		return false;
	}

	return canNode->transmittedFrameCb(nodeHandle, p_eventData, nullptr);
}

static bool staticStateChangedCb(twai_node_handle_t nodeHandle, const twai_state_change_event_data_t* p_eventData, void* p_userCtx)
{
	// Get the class context from the user arguments
	auto canNode = static_cast<Can*>(p_userCtx);
	if (canNode == nullptr) {
		return false;
	}

	return canNode->stateChangedCb(nodeHandle, p_eventData, nullptr);
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

	nodeCallbacks_.on_tx_done = staticTransmittedFrameCb;
	nodeCallbacks_.on_rx_done = staticReceivedFrameCb;
	nodeCallbacks_.on_state_change = staticStateChangedCb;
	nodeCallbacks_.on_error = nullptr;
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
	} else {
		ESP_LOGI(TAG, "Enabled CAN node");
	}

	return enabled_;
}

void Can::registerRxCbQueue(QueueHandle_t* queueHandle)
{
	if (queueHandle == nullptr) {
		return;
	}

	rxCbQueueHandles_.push_back(queueHandle);
}

void Can::deregisterRxCbQueue(const QueueHandle_t* queueHandle)
{
	if (queueHandle == nullptr) {
		return;
	}

	// Erase the handle from the vector
	for (uint32_t i = 0; i < rxCbQueueHandles_.size(); i++) {
		if (rxCbQueueHandles_.at(i) != queueHandle) {
			continue;
		}

		rxCbQueueHandles_.erase(rxCbQueueHandles_.begin() + i);
		return;
	}
}

void Can::queueFrame(Frame& canFrame)
{
	if (!enabled_) {
		return;
	}

	canFrame.transmitting = true;
	canFrame.generateTwaiFrame();
	pendingFrames_.push_back(canFrame);

	if (twai_node_transmit(nodeHandle_, &canFrame.twaiFrame, CAN_SEND_TIMEOUT_MS) != ESP_OK) {
		ESP_LOGW(TAG, "Failed to queue CAN frame");
	}
}

/*
 *	Private Callback functions implementations
 */
bool Can::receivedFrameCb(twai_node_handle_t nodeHandle, const twai_rx_done_event_data_t* p_eventData, void* p_userCtx)
{
	if (nodeHandle != nodeHandle_) {
		return false;
	}

	if (p_eventData == nullptr) {
		return false;
	}

	// Get the frame
	twai_frame_t twaiFrame;
	twai_node_receive_from_isr(nodeHandle, &twaiFrame);
	Frame canFrame;
	canFrame.fromTwaiFrame(twaiFrame);

	// Debug logging
	//esp_rom_printf("Received frame with message id %d with buffer: ", twaiFrame.header.id);
	//for (uint16_t i = 0; i < twaiFrame.buffer_len; i++) {
	//	esp_rom_printf("%d ", twaiFrame.buffer[i]);
	//}
	//esp_rom_printf("\n");

	if (rxCbQueueHandles_.empty()) {
		return false;
	}

	// Notify all callback queues
	BaseType_t xHigherPriorityTaskWoken;
	for (const auto& queueHandle : rxCbQueueHandles_) {
		if (queueHandle == nullptr) {
			continue;
		}

		xQueueSendFromISR(*queueHandle, &canFrame, &xHigherPriorityTaskWoken);
	}

	return xHigherPriorityTaskWoken;
}

bool Can::transmittedFrameCb(twai_node_handle_t nodeHandle, const twai_tx_done_event_data_t* p_eventData, void* p_userCtx)
{
	if (nodeHandle != nodeHandle_) {
		return false;
	}

	if (p_eventData == nullptr) {
		return false;
	}

	// Debug logging
	//esp_rom_printf("Transmitted frame with message id %d with buffer: ", p_eventData->done_tx_frame->header.id);
	//for (uint16_t i = 0; i < p_eventData->done_tx_frame->buffer_len; i++) {
	//	esp_rom_printf("%d ", p_eventData->done_tx_frame->buffer[i]);
	//}
	//esp_rom_printf("\n");

	// Free memory
	for (uint32_t i = 0; i < pendingFrames_.size(); i++) {
		if (!pendingFrames_.at(i).transmitting || pendingFrames_.at(i).twaiFrame.header.id != p_eventData->done_tx_frame->header.id) {
			continue;
		}

		free(pendingFrames_.at(i).twaiFrame.buffer);

		pendingFrames_.erase(pendingFrames_.begin() + i);

		return false;
	}

	return false;
}

bool Can::stateChangedCb(twai_node_handle_t nodeHandle, const twai_state_change_event_data_t* p_eventData, void* p_userCtx)
{
	if (p_eventData == nullptr) {
		return false;
	}

	if (p_eventData->new_sta != TWAI_ERROR_BUS_OFF) {
		return false;
	}

	esp_rom_printf("State changed from %d to %d\n", p_eventData->old_sta, p_eventData->new_sta);
	// esp_rom_printf("An error occurred. Arbitration lost: %d \n Bit error: %d \n Form error: %d \n Stuff error: %d \n ACK error: %d \n val: %d", edata->err_flags.arb_lost, edata->err_flags.bit_err, edata->err_flags.form_err, edata->err_flags.stuff_err, edata->err_flags.ack_err, edata->err_flags.val);

	twai_node_recover(nodeHandle_);

	return false;
}
