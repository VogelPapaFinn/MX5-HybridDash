#include "StateMachine/States/WaitingForConfiguration.hpp"

// Project includes
#include "StateMachine/StateEvents.hpp"

// espidf includes
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "WaitingForConfiguration";

/*
 *   Public Function Implementations
 */
WaitingForConfiguration::WaitingForConfiguration(Can* p_can) : MessageReceiver(p_can) {}

void WaitingForConfiguration::enter()
{
	ESP_LOGI(TAG, "Entered WaitingForConfiguration State");

	startCanRxTask();
}

void WaitingForConfiguration::quit() { stopCanRxTask(); }

/*
 *   Private Function Implementations
 */
void WaitingForConfiguration::handleCanFrame(const Can::Frame_t& frame)
{
	ESP_LOGI(TAG, "Received CAN frame: %s", frame.toString().c_str());

	if (frame.function == CanFrameGroups::FUNC::ENTER_CONFIGURATION_MODE) {
		// Enter the Configuration state
		esp_event_post(STATE_EVENT_BASE, STATE_EVENT_ID::ENTER_CONFIGURATION_MODE, nullptr, 0, portMAX_DELAY);
		return;
	}
}
