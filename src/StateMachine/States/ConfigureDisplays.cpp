#include "StateMachine/States/ConfigureDisplays.hpp"

// Project includes
#include "StateMachine/StateEvents.hpp"

// espidf includes
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr auto TAG = "ConfigureDisplays";

/*
 *   Public Function Implementations
 */
ConfigureDisplays::ConfigureDisplays(Can* p_can, std::vector<Display>* p_displays)
{
	can_ = p_can;
	displays_ = p_displays;

	currDisplay_ = displays_->begin();
}

void ConfigureDisplays::enter()
{
	for (auto& d : *displays_) {
		d.turnOff();
	}

	startCanRxTask();
	enterNextState();
}

void ConfigureDisplays::quit()
{
	stopTimeoutTimer();
	stopCanRxTask();
}

/*
 *   Private Function Implementations
 */
void ConfigureDisplays::startTimeoutTimer()
{
	if (timeoutTimer_ == nullptr) {
		timeoutTimer_ = xTimerCreate(
			"UpdateTimeoutTimer", pdMS_TO_TICKS(60000), pdFALSE, nullptr, [](TimerHandle_t)
			{ esp_event_post(STATE_EVENT_BASE, STATE_EVENT_ID::UPDATE_FAILED, nullptr, 0, portMAX_DELAY); });
	}

	xTimerStart(timeoutTimer_, 0);
}

void ConfigureDisplays::stopTimeoutTimer()
{
	if (timeoutTimer_ == nullptr) {
		return;
	}

	xTimerStop(timeoutTimer_, 0);
	xTimerDelete(timeoutTimer_, 0);
	timeoutTimer_ = nullptr;
}

void ConfigureDisplays::startCanRxTask()
{
	if (xTaskCreate(canRxTask, "UpdateDisplaysCanRxTask", 4096, this, 2, &canRxTask_) == pdPASS) {
		can_->attach(this, CanFrameGroups::GROUP::UPDATE_DISPLAY);

		// Success
		return;
	}

	// Fail
	canRxTask_ = nullptr;
	ESP_LOGW(TAG, "Failed to start task which receives CAN frames");
}

void ConfigureDisplays::stopCanRxTask()
{
	if (canRxTask_ == nullptr) {
		return;
	}

	can_->detach(this, CanFrameGroups::GROUP::CONFIGURE_DISPLAY);


	vTaskDelete(canRxTask_);
	canRxTask_ = nullptr;
}

void ConfigureDisplays::enterNextState()
{
	substate_++;
	startTimeoutTimer();

	switch (substate_) {
		case PREPARE_NEXT_DISPLAY: {

		} break;

		case WAITING_FOR_DISPLAY: {
		} break;

		case SET_CAN_ID: {
		} break;

		case SET_SCREEN: {
		} break;

		case SET_ROTATION: {
		} break;

		case FINISHED: {
		} break;

		default: {
		}
	}
}

void ConfigureDisplays::handleCanFrame(const Can::Frame_t& frame)
{
	// Ignore irrelevant frames
	if (frame.group != CanFrameGroups::GROUP::CONFIGURE_DISPLAY) {
		return;
	}

	switch (substate_) {
		case PREPARE_NEXT_DISPLAY: {
		} break;

		case WAITING_FOR_DISPLAY: {
		} break;

		case SET_CAN_ID: {
		} break;

		case SET_SCREEN: {
		} break;

		case SET_ROTATION: {
		} break;

		case FINISHED: {
		} break;

		default: {
		}
	}
}
