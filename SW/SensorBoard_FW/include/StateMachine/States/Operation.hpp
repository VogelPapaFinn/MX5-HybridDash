#pragma once

// Project includes
#include "KLine/KLine.hpp"
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
			  SensorBroadcaster* p_sensorBroadcast, KLine* p_kline);

	void enter() override;

	void quit() override;

private:
	SwSensorReader* swSensorReader_ = nullptr;
	HwSensorReader* hwSensorReader_ = nullptr;
	SensorLogger* sensorLogger_ = nullptr;
	SensorBroadcaster* sensorBroadcaster_ = nullptr;
	KLine* kline_ = nullptr;
};
