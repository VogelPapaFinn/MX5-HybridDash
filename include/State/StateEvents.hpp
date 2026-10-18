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
	FINISHED,

	/*
	 *	Operation
	 */

	/*
	 *	ConfigureDisplays
	 */

	/*
	 *	UpdateDisplays
	 */
};
