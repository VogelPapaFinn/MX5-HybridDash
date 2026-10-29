#pragma once

// Project includes
#include "Sensors/HwSensorReader.hpp"
#include "Sensors/SensorBroadcaster.hpp"
#include "Sensors/SensorLogger.hpp"
#include "StateMachine/State.hpp"


/*
 *	Class implementation
 */
class Operation : public State
{
public:
	Operation(HwSensorReader* p_hwSensorReader, SensorLogger* p_sensorLogger, SensorBroadcaster* p_sensorBroadcast);

	void enter() override;

	void quit() override;

private:
	HwSensorReader* hwSensorReader_ = nullptr;
	SensorLogger* sensorLogger_ = nullptr;
	SensorBroadcaster* sensorBroadcaster_ = nullptr;
};
