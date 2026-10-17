#pragma once

// Project includes
#include "ActiveSensor.hpp"

// C++ includes
#include <list>

// espidf includes
#include "freertos/FreeRTOS.h"

class Rpm : public ActiveSensor
{
public:
	Rpm();

	int get() override;

	/*
	 *	Public Callback functions
	 */
	void cb() override;

private:
	/*
	 *	Private Functions
	 */
	bool isEngineRunning(int64_t newestTriggerTime);

	uint16_t calculateRpm(int64_t newestTriggerTime, int64_t previousTriggerTime);

	uint16_t getMedianElement(std::list<uint16_t>& rpms);

	/*
	 *	Private Variables
	 */
	int64_t previousTriggerTime_ = 0;
	int64_t newestTriggerTime_ = 0;

	std::list<uint16_t> lastRpm_ = {0, 0, 0};
};
