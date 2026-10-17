#pragma once

// Project includes
#include "ActiveSensor.hpp"

// espidf includes
#include "freertos/FreeRTOS.h"

class Speed : public ActiveSensor
{
public:
	Speed();

	int get() override;

	/*
	 *	Public Callback functions
	 */
	IRAM_ATTR void cb() override;

private:
	/*
	 *	Private Functions
	 */
	bool isCarMoving(int64_t newestTriggerTime);

	uint8_t calculateKmh(int64_t newestTriggerTime, int64_t previousTriggerTime);

	/*
	 *	Private Variables
	 */
	volatile int64_t previousTriggerTime_ = 0;
	volatile int64_t newestTriggerTime_ = 0;

	uint8_t kmh_ = 0;
};
