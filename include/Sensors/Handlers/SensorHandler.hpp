#pragma once

// Project includes
#include "SensorsData.hpp"

// espidf includes
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"

class SensorHandler
{
public:
	SensorHandler(adc_oneshot_unit_handle_t p_adc1, SensorsData* p_sensorsData);

	void start();

	void stop();

	/*
	 *   Public struct
	 */
	struct TaskStuff
	{
		std::unordered_map<SENSOR, int>* sensorData = nullptr;
		SemaphoreHandle_t sensorDataMutex = nullptr;
		adc_oneshot_unit_handle_t adc = nullptr;
		SensorsData* sensorsData = nullptr;
	};

private:
	/*
	 *	Private Functions
	 */
	void startReadThread();
	void stopReadThread();

	void startWriteThread();
	void stopWriteThread();

	/*
	 *	Private Variables
	 */
	std::unordered_map<SENSOR, int> newSensorData_;

	SemaphoreHandle_t sensorDataMutex_ = nullptr;

	TaskHandle_t updateSensorReadTask_ = nullptr;

	TaskHandle_t sensorWriteTask_ = nullptr;

	adc_oneshot_unit_handle_t adc1_ = nullptr;

	SensorsData* sensorsData_ = nullptr;

	TaskStuff taskStuff_;
};
