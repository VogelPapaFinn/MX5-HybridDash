#include "StateMachine/States/Operation.hpp"

// espidf includes
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "Operation";

/*
 *	Public Function implementations
 */
Operation::Operation(SensorHandler* p_sensorHandler) { sensorHandler_ = p_sensorHandler; }

void Operation::enter()
{
	ESP_LOGI(TAG, "Entering Operation state");
	sensorHandler_->start();
}

void Operation::quit()
{
	ESP_LOGI(TAG, "Quitting Operation state");
	sensorHandler_->stop();
}
