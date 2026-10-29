#pragma once

// espidf includes
#include "driver/gpio.h"
#include "esp_attr.h"

class ActiveSensor
{
public:
	ActiveSensor(gpio_num_t gpio, const gpio_int_type_t& triggeringEdge);

	~ActiveSensor();

	virtual int get() = 0;

	/*
	 *	Public Callback functions
	 */
	IRAM_ATTR virtual void cb();

protected:
	/*
	 *	Private Variables
	 */
	gpio_num_t gpio_ = GPIO_NUM_NC;
};
