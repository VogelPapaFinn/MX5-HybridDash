#include "Sensors/HwSensor/LeftIndicator.hpp"

// espidf includes
#include "esp_event.h"
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "LeftIndicator";

/*
 *	Public Function Implementations
 */
LeftIndicator::LeftIndicator() : ActiveSensor(GPIO_NUM_15, GPIO_INTR_ANYEDGE) {}

int LeftIndicator::get() { return active_; }

void LeftIndicator::cb() { active_ = gpio_get_level(gpio_) == 0; }
