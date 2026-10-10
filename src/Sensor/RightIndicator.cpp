#include "Sensor/RightIndicator.hpp"

// Project includes
#include "Events.hpp"

// espidf includes
#include "esp_event.h"
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "RightIndicator";

constexpr unsigned int ALLOW_CHANGE_AFTER_US = 50000;

/*
 *	Public Function Implementations
 */
RightIndicator::RightIndicator() : ActiveSensor(SENSOR::TYPE::RIGHT_INDICATOR, GPIO_NUM_7, GPIO_INTR_ANYEDGE)
{
	gpio_set_pull_mode(gpio_, GPIO_FLOATING);
}

int RightIndicator::get() { return active_; }

void RightIndicator::cb()
{
	active_ = gpio_get_level(gpio_) == 0;
	notifyAboutNewValue();
}

void RightIndicator::notifyAboutNewValue()
{
	esp_event_isr_post(SYSTEM_EVENT_BASE, RIGHT_INDICATOR_ACTIVE_CHANGED, &active_, sizeof(active_), nullptr);
}
