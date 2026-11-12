#include "Sensors/HwSensor/ActiveSensor.hpp"

// espidf includes
#include "esp_event.h"
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "ActiveSensor";
/*
 *	Public Function Implementations
 */
ActiveSensor::ActiveSensor(const gpio_num_t gpio, const gpio_int_type_t& triggeringEdge,
						   void (*p_isrCallback)(ActiveSensor* p_sensor))
{
	isrCb_ = p_isrCallback;

	// Setup the GPIO
	gpio_ = gpio;
	gpio_set_direction(gpio_, GPIO_MODE_INPUT);
	gpio_set_intr_type(gpio_, triggeringEdge);

	// Enable the ISR
	if (gpio_isr_handler_add(gpio_, gpioIsr, this) != ESP_OK) {
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

void ActiveSensor::setTaskToNotifyOnChange(TaskHandle_t p_task) { taskToNotify_ = p_task; }

/*
 *	Private Function Implementations
 */
IRAM_ATTR void ActiveSensor::gpioIsr(void* p_arg)
{
	if (p_arg == nullptr) {
		return;
	}

	ActiveSensor* instance = static_cast<ActiveSensor*>(p_arg);
	if (instance->isrCb_ != nullptr) {
		instance->isrCb_(instance);
	}

	if (instance->taskToNotify_ != nullptr) {
		BaseType_t higherTaskWoken = pdFALSE;
		xTaskNotifyFromISR(instance->taskToNotify_, 0b01, eSetBits, &higherTaskWoken);
		portYIELD_FROM_ISR(higherTaskWoken);
	}
}
