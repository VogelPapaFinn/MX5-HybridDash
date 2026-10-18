#pragma once

// Project includes
#include "Can.hpp"
#include "Sensors/Sensors.hpp"

// C++ includes
#include <unordered_map>

// espidf includes
#include "freertos/FreeRTOS.h"

/*
 *  constexpr
 */
constexpr auto MAX_WAITING_TIME_FOR_WRITE_ACCESS_MS = 50;

/*
 *   Class Implementations
 */
class SensorsData
{
public:
	SensorsData(Can* p_can);

	std::unordered_map<SENSOR, int> getSnapshot();

	std::unordered_map<SENSOR, int>* getWriteAccess();

	void returnWriteAccess();

	void startBroadcasting();

	void stopBroadcasting();

private:
	/*
	 *	Private Variables
	 */
	Can* can_ = nullptr;

	std::unordered_map<SENSOR, int> sensorsData_;

	SemaphoreHandle_t writeMutex_ = nullptr;

	TaskHandle_t broadcastTaskHandle_ = nullptr;
};
