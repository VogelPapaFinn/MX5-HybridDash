#pragma once

// C++ includes
#include <functional>

// espidf includes
#include "freertos/FreeRTOS.h"

class TimeoutTimer
{
public:
	TimeoutTimer() = delete;

	TimeoutTimer(unsigned int ms, std::function<void()> callOnTimeout);

	~TimeoutTimer();

	void start();

	void stop();

private:
	/*
	 *	Private Functions
	 */
	static void timeout(TimerHandle_t p_timer);

	/*
	 *	Private Variables
	 */
	std::function<void()> cb_;

	TimerHandle_t timer_ = nullptr;
};
