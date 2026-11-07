#pragma once

// espidf includes
#include "driver/gpio.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"

class ActiveSensor
{
public:
	ActiveSensor(gpio_num_t gpio, const gpio_int_type_t& triggeringEdge);

	~ActiveSensor();

	virtual int get() = 0;

	void setTaskToNotifyOnChange(TaskHandle_t p_task);

protected:
	/*
	 *	Private functions
	 */
	static IRAM_ATTR void gpioIsr(void* p_arg);

	IRAM_ATTR virtual void cb();

	/*
	 *	Private Variables
	 */
	gpio_num_t gpio_ = GPIO_NUM_NC;

	TaskHandle_t taskToNotify_ = nullptr;
};
