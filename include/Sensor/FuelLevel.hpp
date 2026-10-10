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

	void notifyAboutNewValue() override;

	/*
	 *	Private Variables
	 */
	std::vector<uint8_t> lastLevels_;

	float smoothedValue_ = -1.0f;
};
