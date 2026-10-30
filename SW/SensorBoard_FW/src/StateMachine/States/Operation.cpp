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
					 SensorBroadcaster* p_sensorBroadcast, KLine* p_kline)
{
	hwSensorReader_ = p_hwSensorReader;
	sensorLogger_ = p_sensorLogger;
	sensorBroadcaster_ = p_sensorBroadcast;
	kline_ = p_kline;
}

void Operation::enter()
{
	ESP_LOGI(TAG, "Entering Operation state");
	hwSensorReader_->start();
	sensorLogger_->start();
	sensorBroadcaster_->start();

	// kline_->init();
	KLineMessage m(0x22, {0x17, 0x38});
	kline_->send(m, [](KLineMessage response) {});
}

void Operation::quit()
{
	ESP_LOGI(TAG, "Quitting Operation state");
	hwSensorReader_->stop();
	sensorLogger_->stop();
	sensorBroadcaster_->stop();
}
