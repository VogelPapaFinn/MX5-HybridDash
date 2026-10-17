#pragma once

// Project includes
#include "SensorsData.hpp"

// espidf includes
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"

class SensorHandler
{
public:
	SensorHandler(adc_oneshot_unit_handle_t p_adc1);

	void start();

	void stop();

private:
	void startReadThread();
	void stopReadThread();

	void startWriteThread();
	void stopWriteThread();

	std::unordered_map<HW_SENSOR, int> newSensorData_;

	SemaphoreHandle_t sensorDataMutex_ = nullptr;

	TaskHandle_t updateSensorReadTask_ = nullptr;

	TaskHandle_t sensorWriteTask_ = nullptr;

	adc_oneshot_unit_handle_t adc1_ = nullptr;
};
