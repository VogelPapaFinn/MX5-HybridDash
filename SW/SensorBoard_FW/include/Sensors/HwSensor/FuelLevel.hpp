#pragma once

// Project includes
#include "PassiveSensor.hpp"
#include "Sensors/Utils/MedianFilter.hpp"

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

	float filterResistance();

	float calculatePercentage();

	/*
	 *	Private Variables
	 */
	// EMA = Exponential Moving Average (Smoothing)
	float levelInPercent_ = 0.0f;

	// Hysterese Filter
	uint8_t displayedLevelInPercent_ = 0;

	MedianFilter<float, 15> medianFilter_;

	bool engineRunning_ = false;
	bool firstRead_ = true;

	nvs_handle_t nvsHandle_ = 0;
};
