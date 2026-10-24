#include "Sensors/SensorValues.hpp"

// espidf includes
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr auto TAG = "SensorValues";

/*
 *   Public Function Implementations
 */

SensorValues::SensorValues()
{
	valuesMutex_ = xSemaphoreCreateMutex();

	// Fill the map with dummy data
	for (unsigned int i = 0; i < static_cast<unsigned int>(SENSOR::END); i++) {
		sensorValues_[static_cast<SENSOR>(i)] = 0;
	}
}

std::unordered_map<SENSOR, int> SensorValues::getSnapshot()
{
	if (xSemaphoreTake(valuesMutex_, portMAX_DELAY) != pdTRUE) {
		return {};
	}

	auto copy = sensorValues_;

	xSemaphoreGive(valuesMutex_);

	return copy;
}

std::unordered_map<SENSOR, int>* SensorValues::getWriteAccess()
{
	if (xSemaphoreTake(valuesMutex_, portMAX_DELAY)) {
		return &sensorValues_;
	}

	return nullptr;
}

void SensorValues::returnWriteAccess() { xSemaphoreGive(valuesMutex_); }
