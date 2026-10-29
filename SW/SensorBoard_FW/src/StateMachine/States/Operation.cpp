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
Operation::Operation(HwSensorReader* p_hwSensorReader, SensorLogger* p_sensorLogger,
					 SensorBroadcaster* p_sensorBroadcast)
{
	hwSensorReader_ = p_hwSensorReader;
	sensorLogger_ = p_sensorLogger;
	sensorBroadcaster_ = p_sensorBroadcast;
}

void Operation::enter()
{
	ESP_LOGI(TAG, "Entering Operation state");
	hwSensorReader_->start();
	sensorLogger_->start();
	sensorBroadcaster_->start();
}

void Operation::quit()
{
	ESP_LOGI(TAG, "Quitting Operation state");
	hwSensorReader_->stop();
	sensorLogger_->stop();
	sensorBroadcaster_->stop();
}
