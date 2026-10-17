#pragma once

// Project includes
#include "PassiveSensor.hpp"

class OilPressure : public PassiveSensor
{
public:
	OilPressure(adc_oneshot_unit_handle_t p_adc);

	int get() override;

protected:
	/*
	 *	Private Functions
	 */
	void specificRead() override;

	/*
	 *	Private Variables
	 */
	bool pressure_ = false;
};
