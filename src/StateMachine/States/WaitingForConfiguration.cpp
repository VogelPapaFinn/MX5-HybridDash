#include "StateMachine/States/WaitingForConfiguration.hpp"

// Project includes
#include "StateMachine/StateEvents.hpp"

/*
 *   Public Function Implementations
 */
WaitingForConfiguration::WaitingForConfiguration(Can* p_can) : MessageReceiver(p_can) { can_ = p_can; }

void WaitingForConfiguration::enter() { startCanRxTask(); }

void WaitingForConfiguration::quit() { stopCanRxTask(); }

/*
 *   Private Function Implementations
 */
void WaitingForConfiguration::handleCanFrame(const Can::Frame_t& frame)
{
	if (frame.function == CanFrameGroups::FUNC::ENTER_CONFIGURATION_MODE) {
		// Enter the Configuration state
		esp_event_post(STATE_EVENT_BASE, STATE_EVENT_ID::ENTER_CONFIGURATION_MODE, nullptr, 0, portMAX_DELAY);
		return;
	}
}
