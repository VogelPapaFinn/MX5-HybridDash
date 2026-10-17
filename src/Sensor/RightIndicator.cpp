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

/*
 *	Public Function Implementations
 */
RightIndicator::RightIndicator() : ActiveSensor(GPIO_NUM_7, GPIO_INTR_ANYEDGE) {}

int RightIndicator::get() { return active_; }

void RightIndicator::cb() { active_ = gpio_get_level(gpio_) == 0; }
