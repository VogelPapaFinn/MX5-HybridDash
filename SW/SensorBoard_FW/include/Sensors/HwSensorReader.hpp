#pragma once

// Project includes
#include "SensorValues.hpp"

// C++ includes
#include <functional>

// espidf includes
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"

class HwSensorReader
{
public:
	HwSensorReader(adc_oneshot_unit_handle_t p_adc1, SensorValues* p_sensorValues, std::function<void()> cb);

	~HwSensorReader();

	void start();

	void stop();

private:
	/*
	 *	Private Functions
	 */
	void startReadThread();
	void stopReadThread();

	void startIndicatorThread();
	void stopIndicatorThread();

	void startWriteThread();
	void stopWriteThread();

	/*
	 *	Private Task Functions
	 */
	static void updatePassiveSensorsTask(void* p_param);

	static void writeSensorsTask(void* p_param);

	static void indicatorTask(void* p_param);

	/*
	 *	Private Variables
	 */
	TaskHandle_t updateSensorReadTask_ = nullptr;
	TaskHandle_t sensorWriteTask_ = nullptr;

	TaskHandle_t indicatorTask_ = nullptr;
	std::function<void()> indicatorChangedCb_ = nullptr;

	std::unordered_map<SENSOR, int> newSensorValues_;
	SemaphoreHandle_t newSensorValuesMutex_ = nullptr;
	adc_oneshot_unit_handle_t adc1_ = nullptr;
	SensorValues* sensorValues_ = nullptr;
};
