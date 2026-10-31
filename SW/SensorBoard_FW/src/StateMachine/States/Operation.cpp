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
Operation::Operation(HwSensorReader* p_hwSensorReader, SwSensorReader* p_swSensorReader, SensorLogger* p_sensorLogger,
					 SensorBroadcaster* p_sensorBroadcast, KLine* p_kline)
{
	swSensorReader_ = p_swSensorReader;
	hwSensorReader_ = p_hwSensorReader;
	sensorLogger_ = p_sensorLogger;
	sensorBroadcaster_ = p_sensorBroadcast;
	kline_ = p_kline;
}

void Operation::enter()
{
	ESP_LOGI(TAG, "Entering Operation state");
	kline_->init();
	hwSensorReader_->start();
	swSensorReader_->start();
	sensorLogger_->start();
	sensorBroadcaster_->start();
}

void Operation::quit()
{
	ESP_LOGI(TAG, "Quitting Operation state");
	hwSensorReader_->stop();
	swSensorReader_->stop();
	sensorLogger_->stop();
	sensorBroadcaster_->stop();
}
