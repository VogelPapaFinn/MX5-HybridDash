#pragma once

// Project includes
#include "SensorValues.hpp"

// espidf includes
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"

class SwSensorReader
{
public:
	SwSensorReader(SensorValues* p_sensorValues);

	void start();

	void stop();

private:
	/*
	 *	Private Functions
	 */
	void startReadThread();
	void stopReadThread();

	/*
	 *	Private Task Functions
	 */
	static void readSensors(void* p_param);

	/*
	 *	Private Variables
	 */
	TaskHandle_t readSensorsTask_ = nullptr;

	SensorValues* sensorValues_ = nullptr;
};
