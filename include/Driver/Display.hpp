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
	/**
	 * \brief Constructor to initialize the display with power GPIO, CAN ID and screen number.
	 *
	 * \param[in] powerGpio Power GPIO pin of the display.
	 * \param[in] canId The CAN ID for the display's receive task.
	 * \param[in] screen The screen number on which the display is connected.
	 * \param[in] rotateBy180 If true, rotates the display by 180 degrees.
	 */
	explicit Display(Can* p_can, gpio_num_t powerGpio, const uint8_t& canId, const uint8_t& screen,
					 const bool& rotateBy180);

	void reset();

	void turnOn() const;

	void turnOff() const;

	void setConfigured(bool configured);

	void bakeConfiguration() const;

	void confirmConfiguration();

	/*
	 *	Public getter-Functions
	 */
	bool hasBeenConfigured() const;

	uint8_t getCanId() const;

	uint8_t getScreen() const;

	bool isRotated() const;

private:
	Can* can_ = nullptr;

	//! CAN ID of the display.
	uint8_t canId_ = 0;

	//! Screen number on which the display is connected.
	uint8_t screen_ = 0;

	//! Flag indicating if the display is rotated by 180 degrees.
	bool rotated_ = false;

	//! Power GPIO pin of the display.
	gpio_num_t powerGpio_ = GPIO_NUM_NC;

	/**
	 * \brief The list of possible CAN IDs for the display's receive task.
	 */
	static std::vector<uint8_t> g_possibleCanIds;

	bool configured_ = false;
};
