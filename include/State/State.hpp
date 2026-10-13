#pragma once

// Project includes
#include "Events.hpp"

// C++ includes
#include <tuple>
#include <vector>

// espidf includes
#include "esp_event_base.h"

class State
{
public:
	/*
	 *	Public enum
	 */
	typedef enum
	{
		UNKNOWN,
		REGISTRATION,
		OPERATION,
		WIFI_OTA_UPDATE
	} TYPE;

	/*
	 *	Public Functions
	 */
	State(TYPE type = UNKNOWN);

	virtual void enter() = 0;

	TYPE getType() const;

protected:
	/*
	 *	Private Variables
	 */
	TYPE type_ = UNKNOWN;

	std::vector<std::tuple<esp_event_base_t, SYSTEM_EVENT_ID, esp_event_handler_instance_t>> eventHandlers_;
};
