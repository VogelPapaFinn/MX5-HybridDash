#pragma once

// Project includes
#include "KLine/KLine.hpp"
#include "SensorValues.hpp"
#include "Sensors/SwSensor/NumericalSensors.hpp"

// C++ includes
#include <atomic>
#include <optional>
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

	void addSensor(SENSOR sensor, int64_t intervalMs);

	void addSensors(const std::vector<SENSOR>& sensors, int64_t intervalMs);

	void removeSensor(SENSOR sensor, int64_t intervalMs);

	void removeAllSensors();

private:
	/*
	 *	Private Structs
	 */
	struct Sensor
	{
		SENSOR type;
		int64_t intervalMs = 0;
		int64_t nextDueMcS = -1;
	};

	/*
	 *	Private Functions
	 */
	void startReadThread();
	void stopReadThread();

	bool doesPidMatch(const KLineMessage& message, const uint16_t& pid);
	int extractSensorValue(KLineMessage& message, uint16_t expectedByteCount);
	void handleSensorResponse(const NumericalSensor sensor, KLineMessage& response);

	std::optional<Sensor> scheduleNextSensor();
	bool waitForStopNotification(TickType_t duration);
	void updateDueTime(Sensor& sensor);
	bool sendRequest(const Sensor& sensor);

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
	std::atomic<int> waitingForResponse_ = 0;
	SemaphoreHandle_t sensorsToReadMutex_ = nullptr;
	std::vector<Sensor> sensorsToRead_;
};
