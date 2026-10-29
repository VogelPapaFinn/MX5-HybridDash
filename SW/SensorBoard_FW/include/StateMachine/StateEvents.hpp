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
	FIRST_STARTUP,
	UPDATE_AVAILABLE,
	STARTUP_FINISHED,

	/*
	 *	Operation
	 */

	/*
	 *	ConfigureDisplays
	 */
	CONFIGURATION_FINISHED,

	/*
	 *	UpdateDisplays
	 */
	UPDATE_FAILED
};
