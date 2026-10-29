#pragma once

// Project includes
#include "SensorValues.hpp"

// espidf includes
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"

class HwSensorReader
{
public:
	HwSensorReader(adc_oneshot_unit_handle_t p_adc1, SensorValues* p_sensorValues);

	void start();

	void stop();

private:
	/*
	 *	Private Functions
	 */
	void startReadThread();
	void stopReadThread();

	void startWriteThread();
	void stopWriteThread();

	/*
	 *	Private Task Functions
	 */
	static void updatePassiveSensorsTask(void* p_param);

	static void writeSensorsTask(void* p_param);

	/*
	 *	Private Variables
	 */
	std::unordered_map<SENSOR, int> newSensorValues_;

	SemaphoreHandle_t newSensorValuesMutex_ = nullptr;

	TaskHandle_t updateSensorReadTask_ = nullptr;

	TaskHandle_t sensorWriteTask_ = nullptr;

	adc_oneshot_unit_handle_t adc1_ = nullptr;

	SensorValues* sensorValues_ = nullptr;
};
