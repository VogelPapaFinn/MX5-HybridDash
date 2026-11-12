#include "Sensors/HwSensor/LeftIndicator.hpp"

// espidf includes
#include "esp_event.h"
#include "esp_log.h"
#include "hal/gpio_ll.h"

/*
 *	constexpr
 */
constexpr auto TAG = "LeftIndicator";

/*
 *	Public Function Implementations
 */
LeftIndicator::LeftIndicator() : ActiveSensor(GPIO_NUM_15, GPIO_INTR_ANYEDGE, isrCallback) {}

int LeftIndicator::get() { return active_; }

IRAM_ATTR void LeftIndicator::isrCallback(ActiveSensor* p_sensor)
{
	auto self = static_cast<LeftIndicator*>(p_sensor);

	// ll needed to prevent crash when config/log is written
	self->active_ = gpio_ll_get_level(&GPIO, self->gpio_) == 0;
}
