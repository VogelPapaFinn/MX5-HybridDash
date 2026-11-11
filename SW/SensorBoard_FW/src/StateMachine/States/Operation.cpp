#include "StateMachine/States/Operation.hpp"

// espidf includes
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "Operation";

constexpr auto SENSORS_TO_READ = {SENSOR::ECU_COOLANT_C, SENSOR::ECU_COOLANT_V, SENSOR::ECU_INJECTION_MS,
								  SENSOR::ECU_RPM, SENSOR::ECU_SPEED};

/*
 *	Public Function Implementations
 */
Operation::Operation(HwSensorReader* p_hwSensorReader, SensorLogger* p_sensorLogger,
					 SensorBroadcaster* p_sensorBroadcast, KLine* p_kline, LogFileServer* p_logFileServer,
					 SensorValues* p_sensorValues, SerialConsole* p_serialConsole) :
	swSensorReader_(p_kline, p_sensorValues)
{
	hwSensorReader_ = p_hwSensorReader;
	sensorLogger_ = p_sensorLogger;
	sensorBroadcaster_ = p_sensorBroadcast;
	kline_ = p_kline;
	logFileServer_ = p_logFileServer;
	sensorValues_ = p_sensorValues;
	serialConsole_ = p_serialConsole;

	swSensorReader_.setSensorsToRead(SENSORS_TO_READ);
}

void Operation::enter()
{
	ESP_LOGI(TAG, "Entering Operation state");
	kline_->init();
	hwSensorReader_->start();
	swSensorReader_.start();
	sensorLogger_->start();
	sensorBroadcaster_->start();
	logFileServer_->start();
	serialConsole_->start();
	startRpmTimer();
}

void Operation::quit()
{
	ESP_LOGI(TAG, "Quitting Operation state");
	stopRpmTimer();
	hwSensorReader_->stop();
	swSensorReader_.stop();
	sensorLogger_->stop();
	sensorBroadcaster_->stop();
	logFileServer_->stop();
	serialConsole_->stop();
}

/*
 *	Private Function Implementations
 */
void Operation::startRpmTimer()
{
	if (rpmTimer_ != nullptr) {
		return;
	}

	rpmTimer_ = xTimerCreate("WaitingForRpm>0", pdMS_TO_TICKS(1000), pdTRUE, this,
							 [](TimerHandle_t p_timer)
							 {
								 if (p_timer == nullptr) {
									 return;
								 }

								 auto state = static_cast<Operation*>(pvTimerGetTimerID(p_timer));

								 // Check if RPM > 0
								 auto sensorValues = state->sensorValues_->getSnapshot();
								 if (sensorValues[SENSOR::RPM] <= 0) {
									 return;
								 }

								 // Stop LogFileServer and Timer
								 state->logFileServer_->stop();
								 xTimerStop(p_timer, 0);
								 xTimerDelete(p_timer, 0);
								 state->rpmTimer_ = nullptr;
							 });

	if (rpmTimer_ == nullptr) {
		ESP_LOGW(TAG, "Failed to start the timer that stops the LogFileServer");
		return;
	}

	xTimerStart(rpmTimer_, 0);
}

void Operation::stopRpmTimer()
{
	if (rpmTimer_ == nullptr) {
		return;
	}

	xTimerStop(rpmTimer_, 0);
	xTimerDelete(rpmTimer_, 0);
	rpmTimer_ = nullptr;
}
