#include "State/Registration.hpp"

// Project includes
#include "Can.hpp"
#include "CanGroupsAndFunctions.hpp"
#include "Driver/Display.hpp"
#include "Events.hpp"

// espidf includes
#include "esp_log.h"

/*
 *	constexpr
 */
const auto TAG = "Registration";

/*
 *	Public Function Implementations
 */
Registration::Registration(Can* p_can, std::vector<Display>* p_displays) : State(State::REGISTRATION)
{
	can_ = p_can;
	displays_ = p_displays;
}

Registration::~Registration()
{
	/*
	 *	Unregister from all events
	 */
	for (const auto& event : eventHandlers_) {
		const auto& base = std::get<0>(event);
		const auto& id = std::get<1>(event);
		const auto& handler = std::get<2>(event);

		esp_event_handler_instance_unregister(base, id, handler);
	}
	eventHandlers_.clear();
}

void Registration::enter()
{
	// Register necessary events on the event loop
	registerToEvents();

	// Start the first display
	displays_->at(0).turnOn();
}

/*
 *	Private Function Implementations
 */
void Registration::registerToEvents()
{
	/*
	 *	Display registered
	 */
	eventHandlers_.push_back(std::make_tuple(SYSTEM_EVENT_BASE, DISPLAY_REGISTERED, esp_event_handler_instance_t()));
	esp_event_handler_instance_register(
		SYSTEM_EVENT_BASE, DISPLAY_REGISTERED,
		[](void* p_state, esp_event_base_t, int32_t, void*)
		{
			/*
			 *	Get the state ptr
			 */
			if (p_state == nullptr) {
				return;
			}

			// Convert it
			Registration* state = static_cast<Registration*>(p_state);

			/*
			 *	Pass the register call
			 */
			state->nextDisplay();
		},
		this, &get<2>(eventHandlers_.back()));
}

void Registration::nextDisplay()
{
	ESP_LOGI(TAG, "Next Display");

	/*
	 *	Complete registration if possible
	 */
	if (++currDisplay >= 1) {
		wakeUpAllDisplays();

		esp_event_post(SYSTEM_EVENT_BASE, REGISTRATION_COMPLETED, nullptr, 0, portMAX_DELAY);

		return;
	}

	/*
	 *	Turn on the next display
	 */
	displays_->at(currDisplay).turnOn();
}

void Registration::wakeUpAllDisplays() const
{
	Can::Frame_t frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = CAN_BROADCAST_ID;
	frame.group = CanFrameGroups::GROUP::CONFIGURATION;
	frame.function = CanFrameGroups::CONFIGURATION::WAKE_UP;
	frame.dataLengthCode = 0;
	frame.answer = false;

	can_->queueFrame(frame);
}
