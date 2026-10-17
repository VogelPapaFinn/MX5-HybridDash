#include "Sensor/ActiveSensor.hpp"

// Project include
#include "Events.hpp"

// espidf includes
#include "esp_event.h"
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "ActiveSensor";

/*
 *	Private Static Callback Functions
 */
static IRAM_ATTR void staticIsr(void* p_arg)
{
	if (p_arg == nullptr) {
		return;
	}

	ActiveSensor* instance = static_cast<ActiveSensor*>(p_arg);
	instance->cb();
}

/*
 *	Public Function Implementations
 */
ActiveSensor::ActiveSensor(const gpio_num_t gpio, const gpio_int_type_t& triggeringEdge)
{
	// Setup the GPIO
	gpio_ = gpio;
	gpio_set_direction(gpio_, GPIO_MODE_INPUT);
	gpio_set_intr_type(gpio_, triggeringEdge);

	// Enable the ISR
	if (gpio_isr_handler_add(gpio_, staticIsr, this) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to enable the ISR");
	}
}

ActiveSensor::~ActiveSensor()
{
	// Disable the ISR
	if (gpio_isr_handler_remove(gpio_) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to disable the ISR");
	}

	// Reset the GPIO direction
	gpio_set_direction(gpio_, GPIO_MODE_DISABLE);
}

void ActiveSensor::cb() {}
