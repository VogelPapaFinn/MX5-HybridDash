#pragma once

// Project includes
#include "Sensor.hpp"

// espidf includes
#include "driver/gpio.h"
#include "esp_attr.h"

class ActiveSensor : public Sensor
{
public:
	ActiveSensor(SENSOR::TYPE type, gpio_num_t gpio, const gpio_int_type_t& triggeringEdge);

	~ActiveSensor() override;

	int get() override;

	/*
	 *	Public Callback functions
	 */
	IRAM_ATTR virtual void cb();

protected:
	/*
	 *	Private Functions
	 */
	void notifyAboutNewValue() override;

	/*
	 *	Private Variables
	 */
	gpio_num_t gpio_ = GPIO_NUM_NC;
};
