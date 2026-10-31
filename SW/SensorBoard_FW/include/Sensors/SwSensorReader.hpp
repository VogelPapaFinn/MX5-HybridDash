#pragma once

// Project includes
#include "KLine/KLine.hpp"
#include "SensorValues.hpp"

// espidf includes
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"

class SwSensorReader
{
public:
	SwSensorReader(KLine* p_kline, SensorValues* p_sensorValues);

	~SwSensorReader();

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
	KLine* kline_ = nullptr;
	SensorValues* sensorValues_ = nullptr;

	TaskHandle_t readSensorsTask_ = nullptr;
};
