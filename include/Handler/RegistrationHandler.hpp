#pragma once

// Project includes
#include "Can.hpp"
#include "Driver/Display.hpp"


class RegistrationHandler
{
public:
	RegistrationHandler(Can* p_can, std::vector<Display>* p_displays);

private:
	/*
	 *	Private Functions
	 */
	void handleRegistration(const Can::Frame_t* p_frame) const;

	void wakeUpAllDisplays() const;

	/*
	 *	Private Variables
	 */
	Can* can_ = nullptr;
	std::vector<Display>* displays_;
};
