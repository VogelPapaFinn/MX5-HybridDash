#pragma once

// Project includes
#include "PassiveSensor.hpp"

// C++ includes
#include <vector>

// espidf includes
#include "nvs.h"

class FuelLevel : public PassiveSensor
{
public:
	FuelLevel(adc_oneshot_unit_handle_t p_adc);

	~FuelLevel();

	int get() override;

	void setEngineRunning(bool running);

protected:
	/*
	 *	Private Functions
	 */
	void specificRead() override;

	/*
	 *	Private Variables
	 */
	// EMA = Exponential Moving Average (Smoothing)
	float emaValue_ = 0.0f;

	bool engineRunning_ = false;

	bool firstRead_ = true;

	nvs_handle_t nvsHandle_ = 0;
};
