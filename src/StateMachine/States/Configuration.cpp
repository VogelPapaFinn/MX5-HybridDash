#include "StateMachine/States/Configuration.hpp"

/*
 *	constexpr
 */
constexpr auto TAG = "Confugration";

/*
 *   Public Function Implementations
 */
Configuration::Configuration(Filesystem* p_fs, Config* p_config, Can* p_can) : MessageReceiver(p_can)
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
			return;
		} break;

		case SET_SCREEN: {
			if (frame.function != CanFrameGroups::FUNC::SET_SCREEN) {
				return;
			}

			screen_ = frame.data[0];
			return;
		} break;

		case SET_ROTATION: {
			if (frame.function != CanFrameGroups::FUNC::SET_ROTATION) {
				return;
			}

			rotation_ = frame.data[0];
			return;
		} break;

		case SAVE_CONFIGURATION: {
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
	frame.target = CAN_BROADCAST_ID;
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
	frame.target = CAN_BROADCAST_ID;
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
	frame.target = CAN_BROADCAST_ID;
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
	frame.target = CAN_BROADCAST_ID;
	frame.group = CanFrameGroups::GROUP::CONFIGURE_DISPLAY;
	frame.function = CanFrameGroups::FUNC::SAVE_CONFIGURATION;
	frame.answer = true;
	can_->queueFrame(frame);
}
