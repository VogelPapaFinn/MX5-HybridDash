#pragma once

// Project includes
#include "Can.hpp"
#include "Sensors/SensorValues.hpp"

class SensorBroadcaster
{
public:
	SensorBroadcaster(Can* p_can, SensorValues* p_sensorValues);

	void start();

	void stop();

private:
	/*
	 *	Private Task Functions
	 */
	static void broadcastTask(void* p_param);

	/*
	 *	Private Variables
	 */
	Can* can_ = nullptr;
	SensorValues* sensorValues_ = nullptr;

	TaskHandle_t broadcastTaskHandle_ = nullptr;
};
