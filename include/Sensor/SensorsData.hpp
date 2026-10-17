#pragma once

// C++ includes
#include <unordered_map>

// espidf includes
#include "freertos/FreeRTOS.h"

/*
 *  constexpr
 */
constexpr auto MAX_WAITING_TIME_FOR_WRITE_ACCESS_MS = 50;

/*
 *   Public Enum
 */
enum class HW_SENSOR
{
	FUEL_LEVEL,
	OIL_PRESSURE,
	WATER_TEMP,
	LEFT_INDICATOR,
	RIGHT_INDICATOR,
	SPEED,
	RPM
};

/*
 *   Class Implementations
 */
class SensorsData
{
public:
	static SensorsData* get();

	std::unordered_map<HW_SENSOR, int> getSnapshot();

	std::unordered_map<HW_SENSOR, int>* getWriteAccess();

	void returnWriteAccess();

private:
	SensorsData();

	static SensorsData* g_instance;

	std::unordered_map<HW_SENSOR, int> sensorsData_;

	SemaphoreHandle_t writeMutex_ = nullptr;
};
