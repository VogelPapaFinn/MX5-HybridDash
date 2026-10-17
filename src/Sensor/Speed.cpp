#include "Sensor/Speed.hpp"

// Project includes
#include "Events.hpp"

// C++ includes
#include <math.h>

// espidf includes
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"

/*
 *	constexpr
 */
constexpr auto TAG = "Speed";

constexpr float MPH_TO_KMH = 1.60934;
constexpr uint16_t DEBOUNCE_TIME_US = 2000;
constexpr float CAR_STANDING_TIMEOUT_S = 0.5f;

/*
 *	Public Function Implementations
 */
Speed::Speed() : ActiveSensor(GPIO_NUM_10, GPIO_INTR_POSEDGE) {}

int Speed::get()
{
	if (!isCarMoving(newestTriggerTime_)) {
		return 0;
	}

	return calculateKmh(newestTriggerTime_, previousTriggerTime_);
}

void Speed::cb()
{
	const int64_t now = esp_timer_get_time();

	// Debouncing, allow a new trigger only after X us passed
	if ((now - newestTriggerTime_) > DEBOUNCE_TIME_US) {
		previousTriggerTime_ = newestTriggerTime_;
		newestTriggerTime_ = now;
	}
}

/*
 *	Private Function Implementations
 */
bool Speed::isCarMoving(int64_t newestTriggerTime)
{
	const int64_t usSinceLastTrigger = esp_timer_get_time() - newestTriggerTime_; // Microseconds
	const float sSinceLastTrigger = static_cast<float>(usSinceLastTrigger) / 1000000.0f; // Seconds

	return sSinceLastTrigger < CAR_STANDING_TIMEOUT_S;
}

uint8_t Speed::calculateKmh(int64_t newestTriggerTime, int64_t previousTriggerTime)
{
	const int64_t usTimeInbetween = newestTriggerTime_ - previousTriggerTime_; // Microseconds
	const float sTimeInbetween = static_cast<float>(usTimeInbetween) / 1000000.0f; // Seconds

	// Error detection
	if (sTimeInbetween <= 0) {
		return 0;
	}

	const float hz = 1.0f / sTimeInbetween;
	const float mph = hz * 0.9f;
	const float kmh = round(mph * MPH_TO_KMH);

	return static_cast<uint8_t>(kmh);
}
