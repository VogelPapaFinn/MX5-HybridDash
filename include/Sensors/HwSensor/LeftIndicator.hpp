#pragma once

// Project includes
#include "ActiveSensor.hpp"

class LeftIndicator : public ActiveSensor
{
public:
	LeftIndicator();

	int get() override;

	/*
	 *	Public Callback functions
	 */
	void cb() override;

private:
	bool active_ = false;
};
