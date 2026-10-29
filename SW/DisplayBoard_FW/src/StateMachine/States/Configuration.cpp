#include "StateMachine/States/Configuration.hpp"

/*
 *	constexpr
 */
constexpr auto TAG = "Configuration";

/*
 *   Public Function Implementations
 */
Configuration::Configuration(Filesystem* p_fs, Config* p_config, Can* p_can) : CanReceiver(p_can)
{
	fs_ = p_fs;
	config_ = p_config;
}

void Configuration::enter()
{
	ESP_LOGI(TAG, "Entered Configuration State");

	startCanRxTask();

	broadcastStateEntering();
}

void Configuration::quit() { stopCanRxTask(); }

/*
 *   Private Function Implementations
 */

void Configuration::handleCanFrame(const Can::Frame_t& frame)
{
	ESP_LOGI(TAG, "Received CAN frame: %s", frame.toString().c_str());

	// Ignore irrelevant frames
	if (frame.group != CanFrameGroups::GROUP::CONFIGURE_DISPLAY) {
		return;
	}

	switch (substate_) {
		case SET_CAN_ID: {
			if (frame.function != CanFrameGroups::FUNC::SET_CAN_ID) {
				return;
			}

			canId_ = frame.data[0];

			ackCanId();
			return;
		} break;

		case SET_SCREEN: {
			if (frame.function != CanFrameGroups::FUNC::SET_SCREEN) {
				return;
			}

			screen_ = frame.data[0];

			ackScreen();
			return;
		} break;

		case SET_ROTATION: {
			if (frame.function != CanFrameGroups::FUNC::SET_ROTATION) {
				return;
			}

			rotation_ = frame.data[0];

			ackRotation();
			return;
		} break;

		case SAVE_CONFIGURATION: {
			if (frame.function != CanFrameGroups::FUNC::SAVE_CONFIGURATION) {
				return;
			}

			saveConfiguration();
			return;
		} break;

		default: {
		}
	}
}

void Configuration::broadcastStateEntering()
{
	Can::Frame_t frame;
	frame.sender = CAN_CONFIGURATION_ID;
	frame.target = CAN_MASTER_ID;
	frame.group = CanFrameGroups::GROUP::CONFIGURE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::ENTER_CONFIGURATION_MODE;
	frame.answer = true;
	can_->queueFrame(frame);

	substate_ = SET_CAN_ID;
}

void Configuration::ackCanId()
{
	Can::Frame_t frame;
	frame.sender = CAN_CONFIGURATION_ID;
	frame.target = CAN_MASTER_ID;
	frame.group = CanFrameGroups::GROUP::CONFIGURE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::SET_CAN_ID;
	frame.answer = true;
	can_->queueFrame(frame);

	substate_ = SET_SCREEN;
}

void Configuration::ackScreen()
{
	Can::Frame_t frame;
	frame.sender = CAN_CONFIGURATION_ID;
	frame.target = CAN_MASTER_ID;
	frame.group = CanFrameGroups::GROUP::CONFIGURE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::SET_SCREEN;
	frame.answer = true;
	can_->queueFrame(frame);

	substate_ = SET_ROTATION;
}

void Configuration::ackRotation()
{
	Can::Frame_t frame;
	frame.sender = CAN_CONFIGURATION_ID;
	frame.target = CAN_MASTER_ID;
	frame.group = CanFrameGroups::GROUP::CONFIGURE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::SET_ROTATION;
	frame.answer = true;
	can_->queueFrame(frame);

	substate_ = SAVE_CONFIGURATION;
}

void Configuration::saveConfiguration()
{
	auto json = config_->getWriteAccess();
	(*json)["canID"] = canId_;
	(*json)["screen"] = screen_;
	(*json)["rotation"] = rotation_;
	config_->returnWriteAccess();
	config_->save();

	Can::Frame_t frame;
	frame.sender = CAN_CONFIGURATION_ID;
	frame.target = CAN_MASTER_ID;
	frame.group = CanFrameGroups::GROUP::CONFIGURE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::SAVE_CONFIGURATION;
	frame.answer = true;
	can_->queueFrame(frame);
}
