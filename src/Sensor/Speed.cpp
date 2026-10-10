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

/*
 *	Public Function Implementations
 */
Speed::Speed() : ActiveSensor(SENSOR::TYPE::SPEED, GPIO_NUM_10, GPIO_INTR_POSEDGE) {}

int Speed::get()
{
	/*
	 * Check if the car is standing
	 */
	portENTER_CRITICAL_ISR(&mux_);
	const int64_t timeSinceLastTrigger = esp_timer_get_time() - fallingEdgeTime_;
	portEXIT_CRITICAL_ISR(&mux_);

	// The car is standing still
	const float secondsSinceLastTrigger = static_cast<float>(timeSinceLastTrigger) / 1000000.0f;
	if (secondsSinceLastTrigger >= 0.5f) {
		return 0;
	}

	/*
	 * Calculate the current speed
	 */
	const int64_t time = fallingEdgeTime_ - lastFallingEdgeTime_;
	const float seconds = static_cast<float>(time) / 1000000.0f;

	// Error detection
	if (seconds <= 0) {
		return 0;
	}

	hz_ = 1.0f / seconds;
	const float mph = hz_ * 0.9f;
	kmh_ = static_cast<int>(round(mph * MPH_TO_KMH));

	return kmh_;
}

void Speed::cb()
{
	portENTER_CRITICAL_ISR(&mux_);

	// Debouncing, allow a new trigger only after X us passed
	const int64_t now = esp_timer_get_time();
	if ((now - fallingEdgeTime_) > DEBOUNCE_TIME_US) {
		lastFallingEdgeTime_ = fallingEdgeTime_;
		fallingEdgeTime_ = now;
	}

	portEXIT_CRITICAL_ISR(&mux_);

	/*
	 *	Notify about new value
	 */
	notifyAboutNewValue();
}

void Speed::notifyAboutNewValue()
{
	esp_event_isr_post(SYSTEM_EVENT_BASE, SPEED_CHANGED, nullptr, 0, nullptr);
}
