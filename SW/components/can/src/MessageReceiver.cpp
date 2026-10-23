#include "MessageReceiver.hpp"

#include <utility>

// espidf includes
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr auto TAG = "MessageReceiver";

/*
 *   Public Function Implementations
 */
MessageReceiver::MessageReceiver(Can* p_can, CanFrameGroups::GROUP group)
{
	can_ = p_can;
	groups_ = {group};

	receivedFramesQueue_ = xQueueCreate(10, sizeof(Can::Frame_t));
}

MessageReceiver::MessageReceiver(Can* p_can, std::vector<CanFrameGroups::GROUP> groups)
{
	can_ = p_can;
	groups_ = std::move(groups);

	receivedFramesQueue_ = xQueueCreate(10, sizeof(Can::Frame_t));
}

MessageReceiver::~MessageReceiver()
{
	stopCanRxTask();
	vQueueDelete(receivedFramesQueue_);
	receivedFramesQueue_ = nullptr;
}

void MessageReceiver::receiveMessage(Can::Frame_t frame)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xQueueSendFromISR(receivedFramesQueue_, &frame, &xHigherPriorityTaskWoken);
}

void MessageReceiver::startCanRxTask()
{
	if (xTaskCreate(handleMessageTask, "CanRxTask", 4096, this, 2, &handleMessageTask_) == pdPASS) {
		for (auto& g : groups_) {
			can_->attach(this, g);
		}

		// Success
		return;
	}

	// Fail
	handleMessageTask_ = nullptr;
	ESP_LOGW(TAG, "Failed to start task which receives CAN frames");
}

void MessageReceiver::stopCanRxTask()
{
	if (handleMessageTask_ == nullptr) {
		return;
	}

	for (auto& g : groups_) {
		can_->detach(this, g);
	}


	vTaskDelete(handleMessageTask_);
	handleMessageTask_ = nullptr;
}

/*
 *   Private Function Implementations
 */
void MessageReceiver::handleMessageTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto messageReceiver = static_cast<MessageReceiver*>(p_param);

	Can::Frame_t frame;
	while (true) {
		if (xQueueReceive(messageReceiver->receivedFramesQueue_, &frame, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		messageReceiver->handleCanFrame(frame);
	}
}
