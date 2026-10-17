#include "Sensor/SensorHandler.hpp"

// Project includes
#include "Sensor/FuelLevel.hpp"
#include "Sensor/LeftIndicator.hpp"
#include "Sensor/OilPressure.hpp"
#include "Sensor/RightIndicator.hpp"
#include "Sensor/Rpm.hpp"
#include "Sensor/Speed.hpp"
#include "Sensor/WaterTemperature.hpp"

// espidf includes
#include <esp_log.h>

/*
 *  constexpr
 */
constexpr auto TAG = "SensorHandler";
constexpr auto PASSIVE_SENSOR_READ_INTERVAL_MS = 1000;
constexpr auto SENSORS_WRITE_INTERVAL_MS = 100;

/*
 *   Private struct
 */
struct TaskStuff
{
	std::unordered_map<HW_SENSOR, int>* sensorData = nullptr;
	SemaphoreHandle_t sensorDataMutex = nullptr;
	adc_oneshot_unit_handle_t adc = nullptr;
};

/*
 *   Task Functions
 */
static void updatePassiveSensorsTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto taskStuff = static_cast<TaskStuff*>(p_param);

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
		(*taskStuff->sensorData)[HW_SENSOR::FUEL_LEVEL] = fL.get();
		(*taskStuff->sensorData)[HW_SENSOR::OIL_PRESSURE] = oP.get();
		(*taskStuff->sensorData)[HW_SENSOR::WATER_TEMP] = wT.get();

		// Return mutex
		xSemaphoreGive(taskStuff->sensorDataMutex);
	}
}

static void writeSensorsTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto taskStuff = static_cast<TaskStuff*>(p_param);
	auto sensorsDataInstance = SensorsData::get();

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
		(*taskStuff->sensorData)[HW_SENSOR::SPEED] = speedVal;
		(*taskStuff->sensorData)[HW_SENSOR::RPM] = rpmVal;
		(*taskStuff->sensorData)[HW_SENSOR::LEFT_INDICATOR] = lIndicVal;
		(*taskStuff->sensorData)[HW_SENSOR::RIGHT_INDICATOR] = rIndicVal;

		// Update values in the SensorsData object
		auto currData = sensorsDataInstance->getWriteAccess();
		if (currData != nullptr) {
			*currData = *taskStuff->sensorData;

			sensorsDataInstance->returnWriteAccess();
		}

		// Return mutex
		xSemaphoreGive(taskStuff->sensorDataMutex);
	}
}

/*
 *   Public Function Implementations
 */
SensorHandler::SensorHandler(adc_oneshot_unit_handle_t p_adc1)
{
	adc1_ = p_adc1;

	sensorDataMutex_ = xSemaphoreCreateMutex();

	// Fill the map with dummy data
	newSensorData_[HW_SENSOR::FUEL_LEVEL] = 0;
	newSensorData_[HW_SENSOR::OIL_PRESSURE] = 0;
	newSensorData_[HW_SENSOR::WATER_TEMP] = 0;
	newSensorData_[HW_SENSOR::LEFT_INDICATOR] = 0;
	newSensorData_[HW_SENSOR::RIGHT_INDICATOR] = 0;
	newSensorData_[HW_SENSOR::SPEED] = 0;
	newSensorData_[HW_SENSOR::RPM] = 0;

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
	TaskStuff taskStuff = {.sensorData = &newSensorData_, .sensorDataMutex = sensorDataMutex_, .adc = adc1_};

	if (xTaskCreate(updatePassiveSensorsTask, "UpdatePassiveSensors", 4096, &taskStuff, 2, &updateSensorReadTask_) ==
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
	TaskStuff taskStuff = {.sensorData = &newSensorData_, .sensorDataMutex = sensorDataMutex_, .adc = adc1_};

	if (xTaskCreate(writeSensorsTask, "WriteSensors", 4096, &taskStuff, 2, &sensorWriteTask_) == pdPASS) {
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
