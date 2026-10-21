#include "StateMachine/Utils/TimeoutTimer.hpp"

/*
 *	Public Function Implementations
 */
TimeoutTimer::TimeoutTimer(unsigned int ms, std::function<void()> callOnTimeout)
{
	cb_ = std::move(callOnTimeout);

	timer_ = xTimerCreate("TimeoutTimer", pdMS_TO_TICKS(ms), pdFALSE, this, timeout);
}

TimeoutTimer::~TimeoutTimer()
{
	stop();
	xTimerDelete(timer_, 0);
}

void TimeoutTimer::start() { xTimerStart(timer_, 0); }

void TimeoutTimer::stop() { xTimerStop(timer_, 0); }

/*
 *	Private Function Implementations
 */
void TimeoutTimer::timeout(TimerHandle_t p_timer)
{
	if (p_timer == nullptr) {
		return;
	}

	auto timeoutTimer = static_cast<TimeoutTimer*>(pvTimerGetTimerID(p_timer));

	timeoutTimer->cb_();
}
