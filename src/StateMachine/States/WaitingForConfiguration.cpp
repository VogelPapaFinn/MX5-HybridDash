#include "StateMachine/States/WaitingForConfiguration.hpp"

// Project includes
#include "StateMachine/StateEvents.hpp"

WaitingForConfiguration::WaitingForConfiguration(Can* p_can) :
	MessageReceiver(p_can, CanFrameGroups::GROUP::CONFIGURE_DISPLAY)
{
	can_ = p_can;
}

void WaitingForConfiguration::enter() { startCanRxTask(); }

void WaitingForConfiguration::quit() { stopCanRxTask(); }

void WaitingForConfiguration::handleCanFrame(const Can::Frame_t& frame)
{
	// Ignore irrelevant frames
	if (frame.group != CanFrameGroups::GROUP::CONFIGURE_DISPLAY) {
		return;
	}

	if (frame.function == CanFrameGroups::CONFIGURE_DISPLAY::ENTER_CONFIGURATION_MODE) {
		// Enter the Configuration state
		esp_event_post(STATE_EVENT_BASE, STATE_EVENT_ID::ENTER_CONFIGURATION_MODE, nullptr, 0, portMAX_DELAY);
		return;
	}
}
