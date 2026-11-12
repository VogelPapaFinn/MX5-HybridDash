#pragma once

// Project includes
#include "KLine/KLine.hpp"
#include "SensorValues.hpp"
#include "Sensors/SwSensor/NumericalSensors.hpp"

// C++ includes
#include <atomic>
#include <vector>

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

	void setSensorsToRead(std::vector<SENSOR> sensors);

	void addSensorToRead(SENSOR sensor);

	void removeSensorFromRead(SENSOR sensor);

	void stopReadingAllSensors();

private:
	/*
	 *	Private Functions
	 */
	void startReadThread();
	void stopReadThread();

	void handleSensorResponse(const NumericalSensor sensor, KLineMessage& response);

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

	std::atomic<bool> sensorsReadTaskRunning_ = false;
	SemaphoreHandle_t sensorsToReadMutex_ = nullptr;
	std::vector<SENSOR> sensorsToRead_;
};
