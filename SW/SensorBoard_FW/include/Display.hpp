#pragma once

// Project includes
#include "Can.hpp"

// C++ includes
#include <thread>
#include <vector>

// espidf includes
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

//! \class Display Class representing an ESP32 display driver.
class Display
{
public:
	explicit Display(gpio_num_t powerGpio, const uint8_t& canId, const uint8_t& screen, const bool& rotateBy180);

	void turnOn() const;

	void turnOff() const;

	/*
	 *	Public getter-Functions
	 */
	uint8_t getCanId() const;

	uint8_t getScreen() const;

	bool isRotated() const;

private:
	//! CAN ID of the display.
	uint8_t canId_ = 0;

	//! Screen number on which the display is connected.
	uint8_t screen_ = 0;

	//! Flag indicating if the display is rotated by 180 degrees.
	bool rotated_ = false;

	//! Power GPIO pin of the display.
	gpio_num_t powerGpio_ = GPIO_NUM_NC;
};
