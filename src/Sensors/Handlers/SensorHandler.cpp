#include "Sensors/Handlers/SensorHandler.hpp"

// Project includes
#include "Sensors/HwSensor/FuelLevel.hpp"
#include "Sensors/HwSensor/LeftIndicator.hpp"
#include "Sensors/HwSensor/OilPressure.hpp"
#include "Sensors/HwSensor/RightIndicator.hpp"
#include "Sensors/HwSensor/Rpm.hpp"
#include "Sensors/HwSensor/Speed.hpp"
#include "Sensors/HwSensor/WaterTemperature.hpp"
#include "Sensors/Sensors.hpp"

// espidf includes
#include <esp_log.h>

/*
 *  constexpr
 */
constexpr auto TAG = "SensorHandler";
constexpr auto PASSIVE_SENSOR_READ_INTERVAL_MS = 1000;
constexpr auto SENSORS_WRITE_INTERVAL_MS = 100;

/*
 *   Task Functions
 */
static void updatePassiveSensorsTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto taskStuff = static_cast<SensorHandler::TaskStuff*>(p_param);

	// Create all sensors
	FuelLevel fL(taskStuff->adc);
	OilPressure oP(taskStuff->adc);
	WaterTemperature wT(taskStuff->adc);

	while (true) {
		vTaskDelay(pdMS_TO_TICKS(PASSIVE_SENSOR_READ_INTERVAL_MS));

		// Read sensors
		fL.read();
		oP.read();
		wT.read();

		// Acquire mutex
		if (xSemaphoreTake(taskStuff->sensorDataMutex, pdMS_TO_TICKS(PASSIVE_SENSOR_READ_INTERVAL_MS / 2)) == pdFALSE) {
			continue;
		}

		// Save new values
		(*taskStuff->sensorData)[SENSOR::FUEL_LEVEL_P] = fL.get();
		(*taskStuff->sensorData)[SENSOR::FUEL_LEVEL_R] = fL.getResistance();
		(*taskStuff->sensorData)[SENSOR::OIL_PRESSURE_B] = oP.get();
		(*taskStuff->sensorData)[SENSOR::OIL_PRESSURE_R] = oP.getResistance();
		(*taskStuff->sensorData)[SENSOR::WATER_TEMP_C] = wT.get();
		(*taskStuff->sensorData)[SENSOR::WATER_TEMP_R] = wT.getResistance();

		// Return mutex
		xSemaphoreGive(taskStuff->sensorDataMutex);
	}
}

static void writeSensorsTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto taskStuff = static_cast<SensorHandler::TaskStuff*>(p_param);

	// Create all isr sensors
	Speed speed;
	Rpm rpm;
	LeftIndicator lIndic;
	RightIndicator rIndic;

	while (true) {
		vTaskDelay(pdMS_TO_TICKS(SENSORS_WRITE_INTERVAL_MS));

		// Get newest sensor values
		const int speedVal = speed.get();
		const int rpmVal = rpm.get();
		const int lIndicVal = lIndic.get();
		const int rIndicVal = rIndic.get();

		// Acquire mutex
		if (xSemaphoreTake(taskStuff->sensorDataMutex, pdMS_TO_TICKS(SENSORS_WRITE_INTERVAL_MS / 2)) == pdFALSE) {
			continue;
		}

		// Update sensor values of the ISR sensors
		(*taskStuff->sensorData)[SENSOR::SPEED] = speedVal;
		(*taskStuff->sensorData)[SENSOR::RPM] = rpmVal;
		(*taskStuff->sensorData)[SENSOR::LEFT_INDICATOR] = lIndicVal;
		(*taskStuff->sensorData)[SENSOR::RIGHT_INDICATOR] = rIndicVal;

		// Update values in the SensorsData object
		auto currData = taskStuff->sensorsData->getWriteAccess();
		if (currData != nullptr) {
			*currData = *taskStuff->sensorData;

			taskStuff->sensorsData->returnWriteAccess();
		}

		// Return mutex
		xSemaphoreGive(taskStuff->sensorDataMutex);
	}
}

/*
 *   Public Function Implementations
 */
SensorHandler::SensorHandler(adc_oneshot_unit_handle_t p_adc1, SensorsData* p_sensorsData)
{
	adc1_ = p_adc1;
	sensorsData_ = p_sensorsData;

	sensorDataMutex_ = xSemaphoreCreateMutex();

	// Fill the map with dummy data
	newSensorData_[SENSOR::FUEL_LEVEL_R] = 0;
	newSensorData_[SENSOR::FUEL_LEVEL_P] = 0;
	newSensorData_[SENSOR::OIL_PRESSURE_R] = 0;
	newSensorData_[SENSOR::OIL_PRESSURE_B] = 0;
	newSensorData_[SENSOR::WATER_TEMP_R] = 0;
	newSensorData_[SENSOR::WATER_TEMP_C] = 0;
	newSensorData_[SENSOR::LEFT_INDICATOR] = 0;
	newSensorData_[SENSOR::RIGHT_INDICATOR] = 0;
	newSensorData_[SENSOR::SPEED] = 0;
	newSensorData_[SENSOR::RPM] = 0;

	// Fill TaskStuff
	taskStuff_ = {
		.sensorData = &newSensorData_, .sensorDataMutex = sensorDataMutex_, .adc = adc1_, .sensorsData = sensorsData_};

	// Install ISR Service, needed for Speed, Rpm and Indicators
	if (gpio_install_isr_service(ESP_INTR_FLAG_IRAM) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to install the ISR service");
	}
}

void SensorHandler::start()
{
	startReadThread();
	startWriteThread();
}

void SensorHandler::stop()
{
	stopWriteThread();
	stopReadThread();
}

/*
 *   Public Function Implementations
 */
void SensorHandler::startReadThread()
{
	if (xTaskCreate(updatePassiveSensorsTask, "UpdatePassiveSensors", 4096, &taskStuff_, 2, &updateSensorReadTask_) ==
		pdPASS) {
		// Success
		return;
	}

	// Fail
	updateSensorReadTask_ = nullptr;
	ESP_LOGW(TAG, "Failed to start task which updates the passive sensor values");
	return;
}

void SensorHandler::stopReadThread()
{
	if (updateSensorReadTask_ == nullptr) {
		return;
	}

	vTaskDelete(updateSensorReadTask_);
	updateSensorReadTask_ = nullptr;
}

void SensorHandler::startWriteThread()
{
	if (xTaskCreate(writeSensorsTask, "WriteSensors", 4096, &taskStuff_, 2, &sensorWriteTask_) == pdPASS) {
		// Success
		return;
	}

	// Fail
	sensorWriteTask_ = nullptr;
	ESP_LOGW(TAG, "Failed to start task which updates the sensor values");
	return;
}

void SensorHandler::stopWriteThread()
{
	if (sensorWriteTask_ == nullptr) {
		return;
	}

	vTaskDelete(sensorWriteTask_);
	sensorWriteTask_ = nullptr;
}
