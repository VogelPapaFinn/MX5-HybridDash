#pragma once

// Project includes
#include "PassiveSensor.hpp"

// C++ includes
#include <vector>

class FuelLevel : public PassiveSensor
{
public:
	FuelLevel(adc_oneshot_unit_handle_t p_adc);

	int get() override;

protected:
	/*
	 *	Private Functions
	 */
	void specificRead() override;

	void calcLevel();

	float getMedianElement(std::vector<uint8_t>& values);

	int getDampenedValue(const float raw);

	/*
	 *	Private Variables
	 */
	std::vector<uint8_t> lastLevels_;
};
