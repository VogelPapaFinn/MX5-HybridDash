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
	/*
	 *	Public structs
	 */
	struct Value
	{
		float value = 0.0f;
		int64_t updateTimestampMcS = -1; // Microseconds
	};

	/*
	 *	Public Functions
	 */
	SensorValues();

	std::unordered_map<SENSOR, Value> getSnapshot();

	std::unordered_map<SENSOR, Value>* getWriteAccess();

	void returnWriteAccess();

private:
	/*
	 *	Private Variables
	 */
	std::unordered_map<SENSOR, Value> sensorValues_;

	SemaphoreHandle_t valuesMutex_ = nullptr;
};
