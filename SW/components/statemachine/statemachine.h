#pragma once

// Project includes
#include "Global.h"

// C includes
#include <stdbool.h>

// FreeRTOS includes
#include <freertos/FreeRTOS.h>

/*
 *	Typedefs
 */

/*
 *	Functions
 */
State_t getCurrentState(void);

void setCurrentState(State_t newState);
