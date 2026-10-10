#include "Sensor/LeftIndicator.hpp"

// Project includes
#include "Events.hpp"

// espidf includes
#include "esp_event.h"
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "LeftIndicator";

constexpr unsigned int ALLOW_CHANGE_AFTER_US = 50000;

/*
 *	Public Function Implementations
 */
LeftIndicator::LeftIndicator() : ActiveSensor(SENSOR::TYPE::LEFT_INDICATOR, GPIO_NUM_15, GPIO_INTR_ANYEDGE)
{
	gpio_set_pull_mode(gpio_, GPIO_FLOATING);
}

int LeftIndicator::get() { return active_; }

void LeftIndicator::cb()
{
	active_ = gpio_get_level(gpio_) == 0;
	notifyAboutNewValue();
}

void LeftIndicator::notifyAboutNewValue()
{
	esp_event_isr_post(SYSTEM_EVENT_BASE, LEFT_INDICATOR_ACTIVE_CHANGED, &active_, sizeof(active_), nullptr);
}
