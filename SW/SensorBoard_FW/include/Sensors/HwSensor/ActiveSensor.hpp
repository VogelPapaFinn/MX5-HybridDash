#pragma once

// espidf includes
#include "driver/gpio.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"

class ActiveSensor
{
public:
	ActiveSensor(gpio_num_t gpio, const gpio_int_type_t& triggeringEdge,
				 void (*p_isrCallback)(ActiveSensor* p_sensor) = nullptr);

	~ActiveSensor();

	virtual int get() = 0;

	void setTaskToNotifyOnChange(TaskHandle_t p_task);

protected:
	/*
	 *	Private functions
	 */
	static IRAM_ATTR void gpioIsr(void* p_arg);

	/*
	 *	Private Variables
	 */
	gpio_num_t gpio_ = GPIO_NUM_NC;

	// Plain function pointer on purpose: std::function and virtual calls would access the flash inside the ISR
	void (*isrCb_)(ActiveSensor* p_sensor) = nullptr;
	TaskHandle_t taskToNotify_ = nullptr;
};
