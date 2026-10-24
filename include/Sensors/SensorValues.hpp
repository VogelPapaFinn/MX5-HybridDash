#pragma once

// Project includes
#include "Sensors/Sensors.hpp"

// C++ includes
#include <unordered_map>

// espidf includes
#include "freertos/FreeRTOS.h"

/*
 *   Class Implementations
 */
class SensorValues
{
public:
	SensorValues();

	std::unordered_map<SENSOR, int> getSnapshot();

	std::unordered_map<SENSOR, int>* getWriteAccess();

	void returnWriteAccess();

private:
	/*
	 *	Private Variables
	 */
	std::unordered_map<SENSOR, int> sensorValues_;

	SemaphoreHandle_t valuesMutex_ = nullptr;
};
