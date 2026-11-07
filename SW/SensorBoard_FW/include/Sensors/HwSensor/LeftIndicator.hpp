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
	 *	Public Callback functions
	 */
	IRAM_ATTR void cb() override;

	/*
	 *	Private Variables
	 */
	bool active_ = false;
};
