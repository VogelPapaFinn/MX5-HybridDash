#include "Sensors/HwSensor/Rpm.hpp"

// C++ includes
#include <math.h>

// espidf includes
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"

/*
 *	constexpr
 */
constexpr auto TAG = "Rpm";
constexpr uint16_t MAX_RPM = 8000;

/*
 *	Public Function Implementations
 */
Rpm::Rpm() : ActiveSensor(GPIO_NUM_9, GPIO_INTR_NEGEDGE) {}

int Rpm::get()
{
	if (!isEngineRunning(newestTriggerTime_)) {
		return 0;
	}

	// Calculate current rpm
	const uint16_t rpm = calculateRpm(newestTriggerTime_, previousTriggerTime_);
	lastRpm_.pop_front();
	lastRpm_.push_back(rpm);

	return getMedianElement(lastRpm_);
}

void Rpm::cb()
{
	const auto now = esp_timer_get_time();

	// With a max of 8000rpm the min time between each trigger is above 2000us
	if ((now - newestTriggerTime_) > 2000) {
		previousTriggerTime_ = newestTriggerTime_;
		newestTriggerTime_ = now;
	}
}

/*
 *	Private Function Implementations
 */
bool Rpm::isEngineRunning(int64_t newestTriggerTime)
{
	return esp_timer_get_time() - newestTriggerTime < 300000; // 300ms
}

uint16_t Rpm::calculateRpm(int64_t newestTriggerTime, int64_t previousTriggerTime)
{
	const int64_t usTimeInbetween = newestTriggerTime - previousTriggerTime;

	// Prevent division by 0
	if (usTimeInbetween == 0) {
		return 0;
	}

	// Calculate frequency
	const float seconds = static_cast<float>(usTimeInbetween) / 1000000.0f;
	const float hz = 1.0f / seconds;
	if (hz <= 0.0f) {
		return 0;
	}

	// Calculate the rpm
	const uint16_t rpm = static_cast<uint16_t>((hz * 60.0f) / 2.0f);
	if (rpm > MAX_RPM) {
		return 0;
	}

	return rpm;
}

uint16_t Rpm::getMedianElement(std::list<uint16_t>& rpms)
{
	auto sortedRpmList = rpms;
	sortedRpmList.sort();

	return *(++sortedRpmList.begin());
}
