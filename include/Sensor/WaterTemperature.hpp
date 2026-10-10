#pragma once

// Project includes
#include "PassiveSensor.hpp"

class WaterTemperature : public PassiveSensor
{
public:
	WaterTemperature(adc_oneshot_unit_handle_t p_adc);

	int get();

protected:
	/*
	 *	Private Functions
	 */
	void specificRead() override;

	void calcTemperature(uint16_t r);

	void notifyAboutNewValue() override;

	/*
	 *	Private Variables
	 */
	int temperature_ = 0;
};
