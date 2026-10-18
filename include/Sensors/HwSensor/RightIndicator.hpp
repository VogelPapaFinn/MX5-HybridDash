#pragma once

// Project includes
#include "ActiveSensor.hpp"

class RightIndicator : public ActiveSensor
{
public:
	RightIndicator();

	int get() override;

	/*
	 *	Public Callback functions
	 */
	void cb() override;

private:
	bool active_ = false;
};
