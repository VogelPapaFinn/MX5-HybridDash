#include "Driver/Display.hpp"

/*
 *	constexpr
 */
constexpr auto TAG = "Display";


/*
 *	Public function implementations
 */
Display::Display(const gpio_num_t powerGpio, const uint8_t& canId, const uint8_t& screen, const bool& rotateBy180)
{
	powerGpio_ = powerGpio;
	if (powerGpio_ != GPIO_NUM_NC) {
		gpio_set_direction(powerGpio_, GPIO_MODE_OUTPUT);
		gpio_set_level(powerGpio_, 0);
	}

	canId_ = canId;
	screen_ = screen;
	rotated_ = rotateBy180;
}

uint8_t Display::getCanId() const { return canId_; }

uint8_t Display::getScreen() const { return screen_; }

bool Display::isRotated() const { return rotated_; }

void Display::turnOn() const
{
	if (powerGpio_ == GPIO_NUM_NC) {
		return;
	}

	gpio_set_level(powerGpio_, 1);
}

void Display::turnOff() const
{
	if (powerGpio_ == GPIO_NUM_NC) {
		return;
	}

	gpio_set_level(powerGpio_, 0);
}
