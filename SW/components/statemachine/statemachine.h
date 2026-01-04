#pragma once

// Project includes
#include "EventQueues.h"

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
uint8_t getCurrentState(void);

void setCurrentState(uint8_t newState);