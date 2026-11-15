#pragma once

// Project includes
#include "FileServer.hpp"
#include "KLine/KLine.hpp"
#include "Sensors/HwSensorReader.hpp"
#include "Sensors/SensorBroadcaster.hpp"
#include "Sensors/SensorLogger.hpp"
#include "Sensors/SwSensorReader.hpp"
#include "SerialConsole/SerialConsole.hpp"
#include "StateMachine/State.hpp"


/*
 *	Class implementation
 */
class Operation : public State
{
public:
	Operation(HwSensorReader* p_hwSensorReader, SensorLogger* p_sensorLogger, SensorBroadcaster* p_sensorBroadcast,
			  KLine* p_kline, FileServer* p_fileServer, SensorValues* p_sensorValues, SerialConsole* p_serialConsole);

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
	HwSensorReader* hwSensorReader_ = nullptr;
	SensorLogger* sensorLogger_ = nullptr;
	SensorBroadcaster* sensorBroadcaster_ = nullptr;
	KLine* kline_ = nullptr;
	SensorValues* sensorValues_ = nullptr;
	FileServer* fileServer_ = nullptr;
	SerialConsole* serialConsole_ = nullptr;

	SwSensorReader swSensorReader_;

	TimerHandle_t rpmTimer_ = nullptr;
};
