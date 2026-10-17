#pragma once

// Project includes
#include "PassiveSensor.hpp"

class WaterTemperature : public PassiveSensor
{
public:
	WaterTemperature(adc_oneshot_unit_handle_t p_adc);

	int get() override;

protected:
	/*
	 *	Private Functions
	 */
	void specificRead() override;

	int calculateTemp(uint16_t r);
};
