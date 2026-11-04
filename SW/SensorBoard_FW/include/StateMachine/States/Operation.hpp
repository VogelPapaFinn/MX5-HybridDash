#pragma once

// Project includes
#include "KLine/KLine.hpp"
#include "LogFileServer.hpp"
#include "Sensors/HwSensorReader.hpp"
#include "Sensors/SensorBroadcaster.hpp"
#include "Sensors/SensorLogger.hpp"
#include "Sensors/SwSensorReader.hpp"
#include "StateMachine/State.hpp"


/*
 *	Class implementation
 */
class Operation : public State
{
public:
	Operation(HwSensorReader* p_hwSensorReader, SwSensorReader* p_swSensorReader, SensorLogger* p_sensorLogger,
			  SensorBroadcaster* p_sensorBroadcast, KLine* p_kline, LogFileServer* p_logFileServer,
			  SensorValues* p_sensorValues);

	void enter() override;

	void quit() override;

private:
	/*
	 *	Private Functions
	 */
	void startRpmTimer();

	void stopRpmTimer();

	/*
	 *	Private Variables
	 */
	SwSensorReader* swSensorReader_ = nullptr;
	HwSensorReader* hwSensorReader_ = nullptr;
	SensorLogger* sensorLogger_ = nullptr;
	SensorBroadcaster* sensorBroadcaster_ = nullptr;
	KLine* kline_ = nullptr;
	SensorValues* sensorValues_ = nullptr;
	LogFileServer* logFileServer_ = nullptr;

	TimerHandle_t rpmTimer_ = nullptr;
};
