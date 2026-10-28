#include "CanReceiver.hpp"

#include <utility>

// espidf includes
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr auto TAG = "CanReceiver";

/*
 *   Public Function Implementations
 */
CanReceiver::CanReceiver(Can* p_can)
{
	can_ = p_can;

	receivedFramesQueue_ = xQueueCreate(10, sizeof(Can::Frame_t));
}

CanReceiver::~CanReceiver()
{
	stopCanRxTask();
	vQueueDelete(receivedFramesQueue_);
	receivedFramesQueue_ = nullptr;
}

void CanReceiver::receiveMessage(Can::Frame_t frame)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xQueueSendFromISR(receivedFramesQueue_, &frame, &xHigherPriorityTaskWoken);
}

void CanReceiver::startCanRxTask()
{
	if (xTaskCreate(handleMessageTask, "CanRxTask", 4096, this, 2, &handleMessageTask_) == pdPASS) {
		can_->attach(this);

		// Success
		return;
	}

	// Fail
	handleMessageTask_ = nullptr;
	ESP_LOGW(TAG, "Failed to start task which receives CAN frames");
}

void CanReceiver::stopCanRxTask()
{
	if (handleMessageTask_ == nullptr) {
		return;
	}

	can_->detach(this);

	vTaskDelete(handleMessageTask_);
	handleMessageTask_ = nullptr;
}

/*
 *   Private Function Implementations
 */
void CanReceiver::handleMessageTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto canReceiver = static_cast<CanReceiver*>(p_param);

	Can::Frame_t frame;
	while (true) {
		if (xQueueReceive(canReceiver->receivedFramesQueue_, &frame, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		canReceiver->handleCanFrame(frame);
	}
}
