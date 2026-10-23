#pragma once

// espidf includes
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(STATE_EVENT_BASE);

/*
 *	Events
 */
enum STATE_EVENT_ID
{
	UNKNOWN = 0,

	/*
	 *	Startup
	 */
	CONFIG_INVALID,
	CONFIG_VALID,

	/*
	 *	WaitingForConfiguration
	 */
	ENTER_CONFIGURATION_MODE,

	/*
	 *	Operation & WaitingForConfiguration
	 */
	ENTER_UPDATE_MODE,

	/*
	 *	UPDATE
	 */
	UPDATE_FAILED,
};
