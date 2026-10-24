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
ConfigureDisplays::ConfigureDisplays(Can* p_can, Config* p_config, std::vector<Display>* p_displays) :
	MessageReceiver(p_can)
{
	config_ = p_config;
	displays_ = p_displays;

	currDisplay_ = displays_->begin();
}

void ConfigureDisplays::enter()
{
	ESP_LOGI(TAG, "Entered ConfigureDisplays State");

	for (auto& d : *displays_) {
		d.turnOff();
	}

	timeoutTimer_ = new TimeoutTimer(
		60000, []() { esp_event_post(STATE_EVENT_BASE, STATE_EVENT_ID::UPDATE_FAILED, nullptr, 0, portMAX_DELAY); });

	startCanRxTask();
	enterNextState();
}

void ConfigureDisplays::quit()
{
	delete (timeoutTimer_);
	timeoutTimer_ = nullptr;

	stopCanRxTask();

	// Update config
	auto json = config_->getWriteAccess();
	(*json)["FirstStartup"] = true;
	config_->returnWriteAccess();
	config_->save();

	// Turn all displays back on
	for (auto& d : *displays_) {
		d.turnOn();
	}
}

/*
 *   Private Function Implementations
 */
void ConfigureDisplays::enterNextState()
{
	substate_++;
	timeoutTimer_->start();

	switch (substate_) {
		case START_DISPLAY: {
			currDisplay_->turnOn();
			waitForDisplay();
		} break;

		case SET_CAN_ID: {
			setCanId();
		} break;

		case SET_SCREEN: {
			setScreen();
		} break;

		case SET_ROTATION: {
			setRotation();
		} break;

		case FINISH_DISPLAY: {
			saveConfiguration();
		} break;

		default: {
		}
	}
}

void ConfigureDisplays::handleCanFrame(const Can::Frame_t& frame)
{
	// Ignore irrelevant frames
	if (frame.group != CanFrameGroups::GROUP::CONFIGURE_DISPLAY || frame.answer == false) {
		return;
	}

	switch (substate_) {
		case START_DISPLAY: {
			if (frame.function != CanFrameGroups::ENTER_CONFIGURATION_MODE) {
				return;
			}

			stopWaitingForDisplay();
			enterNextState();
			return;
		} break;

		case SET_CAN_ID: {
			if (frame.function != CanFrameGroups::SET_CAN_ID) {
				return;
			}

			enterNextState();
			return;
		} break;

		case SET_SCREEN: {
			if (frame.function != CanFrameGroups::SET_SCREEN) {
				return;
			}

			enterNextState();
			return;
		} break;

		case SET_ROTATION: {
			if (frame.function != CanFrameGroups::SET_ROTATION) {
				return;
			}

			enterNextState();
			return;
		} break;

		case FINISH_DISPLAY: {
			if (frame.function != CanFrameGroups::SAVE_CONFIGURATION) {
				return;
			}

			currDisplay_->turnOff();

			if (currDisplay_ != displays_->end()) {
				currDisplay_++;
				substate_ = 0; // Start from the beginning
				enterNextState();
				return;
			}

			esp_event_post(STATE_EVENT_BASE, STATE_EVENT_ID::CONFIGURATION_FINISHED, nullptr, 0, portMAX_DELAY);

			return;
		} break;

		default: {
		}
	}
}

/*
 *	START_DISPLAY
 */
void ConfigureDisplays::waitForDisplay()
{
	/*
	 *	Build Task function
	 */
	auto waitForDisplayLambda = [](void* p_param)
	{
		if (p_param == nullptr) {
			vTaskDelete(nullptr);
		}

		auto can = static_cast<Can*>(p_param);

		// Build request frame
		Can::Frame_t frame;
		frame.sender = CAN_MASTER_ID;
		frame.target = CAN_BROADCAST_ID;
		frame.group = CanFrameGroups::GROUP::CONFIGURE_DISPLAY;
		frame.function = CanFrameGroups::FUNC::ENTER_CONFIGURATION_MODE;
		frame.dataLengthCode = 0;

		while (true) {
			vTaskDelay(pdMS_TO_TICKS(100));

			can->queueFrame(frame);
		}
	};

	/*
	 *	Start Task
	 */
	if (xTaskCreate(waitForDisplayLambda, "WaitForDisplay", 4096, can_, 2, &waitingForDisplayTask_) == pdFALSE) {
		ESP_LOGE(TAG, "Failed to start Task \'WaitForDisplay\'");
		return;
	}
}

/*
 *	START_DISPLAY
 */
void ConfigureDisplays::stopWaitingForDisplay()
{
	if (waitingForDisplayTask_ == nullptr) {
		return;
	}

	vTaskDelete(waitingForDisplayTask_);
	waitingForDisplayTask_ = nullptr;
}

/*
 *	SET_CAN_ID
 */
void ConfigureDisplays::setCanId()
{
	Can::Frame_t frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = CAN_BROADCAST_ID;
	frame.group = CanFrameGroups::GROUP::CONFIGURE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::SET_CAN_ID;
	frame.dataLengthCode = 1;
	frame.data[0] = currDisplay_->getCanId();

	can_->queueFrame(frame);
}

/*
 *	SET_SCREEN
 */
void ConfigureDisplays::setScreen()
{
	Can::Frame_t frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = CAN_BROADCAST_ID;
	frame.group = CanFrameGroups::GROUP::CONFIGURE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::SET_SCREEN;
	frame.dataLengthCode = 1;
	frame.data[0] = currDisplay_->getScreen();

	can_->queueFrame(frame);
}

/*
 *	SET_ROTATION
 */
void ConfigureDisplays::setRotation()
{
	Can::Frame_t frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = CAN_BROADCAST_ID;
	frame.group = CanFrameGroups::GROUP::CONFIGURE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::SET_ROTATION;
	frame.dataLengthCode = 1;
	frame.data[0] = currDisplay_->isRotated();

	can_->queueFrame(frame);
}

/*
 *	FINISH_DISPLAY
 */
void ConfigureDisplays::saveConfiguration()
{
	Can::Frame_t frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = CAN_BROADCAST_ID;
	frame.group = CanFrameGroups::GROUP::CONFIGURE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::SAVE_CONFIGURATION;
	frame.dataLengthCode = 0;

	can_->queueFrame(frame);
}

/*
 *   Private Task Function Implementations
 */
void ConfigureDisplays::canRxTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto state = static_cast<ConfigureDisplays*>(p_param);

	Can::Frame_t frame;
	while (true) {
		if (xQueueReceive(state->receivedFramesQueue_, &frame, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		state->handleCanFrame(frame);
	}
}
