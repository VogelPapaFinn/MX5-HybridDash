#include "Driver/Display.hpp"

// Project includes
#include "Can.hpp"
#include "CanGroupsAndFunctions.hpp"

/*
 *	Static Member Variables
 */
std::vector<uint8_t> Display::g_possibleCanIds = {1, 2, 3};

/*
 *	constexpr
 */
constexpr auto TAG = "Display";


/*
 *	Public function implementations
 */
Display::Display(Can* p_can, const gpio_num_t powerGpio, const uint8_t& canId, const uint8_t& screen,
				 const bool& rotateBy180)
{
	can_ = p_can;

	powerGpio_ = powerGpio;
	if (powerGpio_ != GPIO_NUM_NC) {
		gpio_set_direction(powerGpio_, GPIO_MODE_OUTPUT);
		gpio_set_level(powerGpio_, 0);
	}

	canId_ = canId;
	screen_ = screen;
	rotated_ = rotateBy180;
}

void Display::reset() { configured_ = false; }

bool Display::hasBeenConfigured() const { return configured_; }

uint8_t Display::getCanId() const { return canId_; }

void Display::bakeConfiguration() const
{
	// Build basic CAN frame
	Can::Frame_t frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = canId_;
	// frame.group = CanFrameGroups::GROUP::CONFIGURATION;
	// frame.function = CanFrameGroups::CONFIGURATION::BAKE_CONFIGURATION;
	frame.dataLengthCode = 3;
	frame.answer = false;

	// Add data
	frame.data[0] = canId_;
	frame.data[1] = screen_;
	frame.data[2] = rotated_;

	// Queue it
	can_->queueFrame(frame);
}

void Display::confirmConfiguration()
{
	configured_ = true;

	Can::Frame_t txFrame;
	txFrame.sender = CAN_MASTER_ID;
	txFrame.target = canId_;
	// txFrame.group = CanFrameGroups::GROUP::CONFIGURATION;
	// txFrame.function = CanFrameGroups::CONFIGURATION::CONFIRM_CONFIGURATION;
	txFrame.answer = false;

	can_->queueFrame(txFrame);
}

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

void Display::setConfigured(bool configured) { configured_ = configured; }
