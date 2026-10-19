#pragma once

// Project includes
#include "Sensors/Handlers/SensorHandler.hpp"
#include "StateMachine/State.hpp"

/*
 *	Class implementation
 */
class Operation : public State
{
public:
	Operation(SensorHandler* p_sensorHandler);

	void enter() override;

	void quit() override;

private:
	SensorHandler* sensorHandler_ = nullptr;
};
