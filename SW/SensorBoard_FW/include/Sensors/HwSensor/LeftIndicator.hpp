#pragma once

// Project includes
#include "ActiveSensor.hpp"

class LeftIndicator : public ActiveSensor
{
public:
	LeftIndicator();

	int get() override;

private:
	/*
	 *	Private Callback Functions
	 */
	static void isrCallback(ActiveSensor* p_sensor);

	/*
	 *	Private Variables
	 */
	bool active_ = false;
};
