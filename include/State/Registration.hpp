#pragma once

// Project include
#include "Can.hpp"
#include "Driver/Display.hpp"
#include "State/State.hpp"


class Registration : public State
{
public:
	Registration(Can* p_can, std::vector<Display>* p_displays);

	~Registration();

	void enter() override;

private:
	/*
	 *	Private Functions
	 */
	void registerToEvents();

	void nextDisplay();

	void wakeUpAllDisplays() const;

	/*
	 *	Private Variables
	 */
	Can* can_ = nullptr;
	std::vector<Display>* displays_ = nullptr;

	uint8_t currDisplay = 0;
};
