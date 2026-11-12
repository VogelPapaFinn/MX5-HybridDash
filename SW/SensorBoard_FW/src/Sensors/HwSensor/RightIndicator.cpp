#include "Sensors/HwSensor/RightIndicator.hpp"

// espidf includes
#include "esp_event.h"
#include "esp_log.h"
#include "hal/gpio_ll.h"

/*
 *	constexpr
 */
constexpr auto TAG = "RightIndicator";

/*
 *	Public Function Implementations
 */
RightIndicator::RightIndicator() : ActiveSensor(GPIO_NUM_7, GPIO_INTR_ANYEDGE, isrCallback) {}

int RightIndicator::get() { return active_; }

/*
 *	Private Function Implementations
 */
IRAM_ATTR void RightIndicator::isrCallback(ActiveSensor* p_sensor)
{
	auto self = static_cast<RightIndicator*>(p_sensor);

	// ll needed to prevent crash when config/log is written
	self->active_ = gpio_ll_get_level(&GPIO, self->gpio_) == 0;
}
