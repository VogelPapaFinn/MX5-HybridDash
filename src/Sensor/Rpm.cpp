#include "Sensor/Rpm.hpp"

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
constexpr auto TAG = "Rpm";
constexpr float MPH_TO_KMH = 1.60934;
constexpr uint16_t MAX_RPM = 8000;

/*
 *	Public Function Implementations
 */
Rpm::Rpm() : ActiveSensor(SENSOR::TYPE::RPM, GPIO_NUM_9, GPIO_INTR_NEGEDGE) {}

int Rpm::get()
{
	// Detect engine shutoff
	uint16_t rpm = 0;
	if (esp_timer_get_time() - fallingEdgeTime_ >= 300000) { // 300ms
		rpm = 0;
		hz_ = 0;
		return rpm;
	}

	portENTER_CRITICAL_ISR(&mux_);
	const int64_t time = fallingEdgeTime_ - lastFallingEdgeTime_;
	portEXIT_CRITICAL_ISR(&mux_);

	// Prevent division by 0
	if (time == 0) {
		return 0;
	}

	// Calculate RPM
	const float seconds = static_cast<float>(time) / 1000000.0f;
	hz_ = 1.0f / seconds;
	if (hz_ <= 0.0f) {
		return 0;
	}

	// Calculate the rpm
	rpm = static_cast<uint16_t>((hz_ * 60.0f) / 2.0f);
	if (rpm > MAX_RPM) {
		rpm = 0;
	}

	/*
	 *	Median filter
	 */
	lastRpm_.pop_front();
	lastRpm_.push_back(rpm);

	auto sortedRpmList = lastRpm_;
	sortedRpmList.sort();

	return *(++sortedRpmList.begin());
}

void Rpm::cb()
{
	/*
	 *	Debouncing
	 */
	const auto now = esp_timer_get_time();
	if ((now - fallingEdgeTime_) > 2000) { // With a max of 8000rpm the min time between each trigger is above 2000us
		portENTER_CRITICAL_ISR(&mux_);
		lastFallingEdgeTime_ = fallingEdgeTime_;
		fallingEdgeTime_ = now;
		portEXIT_CRITICAL_ISR(&mux_);
	}

	/*
	 *	Notify about the new value
	 */
	notifyAboutNewValue();
}

void Rpm::notifyAboutNewValue()
{
	esp_event_isr_post(SYSTEM_EVENT_BASE, RPM_CHANGED, nullptr, 0, nullptr);
}
