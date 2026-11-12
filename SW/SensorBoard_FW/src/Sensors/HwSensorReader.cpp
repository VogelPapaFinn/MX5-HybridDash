#include "Sensors/HwSensorReader.hpp"

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

#include <utility>

/*
 *  constexpr
 */
constexpr auto TAG = "HwSensorReader";
constexpr auto PASSIVE_SENSOR_READ_INTERVAL_MS = 1000;
constexpr auto SENSORS_WRITE_INTERVAL_MS = 100;

/*
 *   Public Function Implementations
 */
HwSensorReader::HwSensorReader(adc_oneshot_unit_handle_t p_adc1, SensorValues* p_sensorValues, std::function<void()> cb)
{
	adc1_ = p_adc1;
	sensorValues_ = p_sensorValues;
	indicatorChangedCb_ = std::move(cb);

	newSensorValuesMutex_ = xSemaphoreCreateMutex();

	// Install ISR Service, needed for Speed, Rpm and Indicators
	if (gpio_install_isr_service(ESP_INTR_FLAG_IRAM) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to install the ISR service");
	}
}

HwSensorReader::~HwSensorReader()
{
	stop();
	gpio_uninstall_isr_service();
}

void HwSensorReader::start()
{
	startReadThread();
	startWriteThread();
	startIndicatorThread();
}

void HwSensorReader::stop()
{
	stopWriteThread();
	stopReadThread();
	stopIndicatorThread();
}

/*
 *   Public Function Implementations
 */
void HwSensorReader::startReadThread()
{
	if (xTaskCreate(updatePassiveSensorsTask, "UpdatePassiveSensors", 4096, this, 2, &updateSensorReadTask_) ==
		pdPASS) {
		// Success
		return;
	}

	// Fail
	updateSensorReadTask_ = nullptr;
	ESP_LOGW(TAG, "Failed to start task which updates the passive sensor values");
}

void HwSensorReader::stopReadThread()
{
	if (updateSensorReadTask_ == nullptr) {
		return;
	}

	xTaskNotifyGive(updateSensorReadTask_);
	updateSensorReadTask_ = nullptr;
}

void HwSensorReader::startIndicatorThread()
{
	if (xTaskCreate(indicatorTask, "IndicatorTask", 2048, this, 2, &indicatorTask_) != pdPASS) {
		// Fail
		indicatorTask_ = nullptr;
		ESP_LOGW(TAG, "Failed to start task which updates the indicators.");
	}
}

void HwSensorReader::stopIndicatorThread()
{
	if (indicatorTask_ != nullptr) {
		xTaskNotify(indicatorTask_, 0b01, eSetBits);
		indicatorTask_ = nullptr;
	}
}

void HwSensorReader::startWriteThread()
{
	if (xTaskCreate(writeSensorsTask, "WriteSensors", 4096, this, 2, &sensorWriteTask_) == pdPASS) {
		// Success
		return;
	}

	// Fail
	sensorWriteTask_ = nullptr;
	ESP_LOGW(TAG, "Failed to start task which updates the sensor values");
	return;
}

void HwSensorReader::stopWriteThread()
{
	if (sensorWriteTask_ == nullptr) {
		return;
	}

	xTaskNotifyGive(sensorWriteTask_);
	sensorWriteTask_ = nullptr;
}

/*
 *   Private Task Function Implementations
 */
void HwSensorReader::updatePassiveSensorsTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	{
		auto hwSensorReader = static_cast<HwSensorReader*>(p_param);

		// Create all sensors
		FuelLevel fL(hwSensorReader->adc1_);
		OilPressure oP(hwSensorReader->adc1_);
		WaterTemperature wT(hwSensorReader->adc1_);

		while (true) {
			// Stop if ordered to
			if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(PASSIVE_SENSOR_READ_INTERVAL_MS)) != 0) {
				break;
			}

			// Read sensors
			fL.read();
			oP.read();
			wT.read();

			// Acquire mutex
			if (xSemaphoreTake(hwSensorReader->newSensorValuesMutex_, pdMS_TO_TICKS(SENSORS_WRITE_INTERVAL_MS / 2)) ==
				pdFALSE) {
				continue;
			}

			// Save new values
			(hwSensorReader->newSensorValues_)[SENSOR::FUEL_LEVEL_P] = fL.get();
			(hwSensorReader->newSensorValues_)[SENSOR::FUEL_LEVEL_R] = fL.getResistance();
			(hwSensorReader->newSensorValues_)[SENSOR::FUEL_LEVEL_MV] = fL.getVoltageMv();
			(hwSensorReader->newSensorValues_)[SENSOR::OIL_PRESSURE_B] = oP.get();
			(hwSensorReader->newSensorValues_)[SENSOR::OIL_PRESSURE_R] = oP.getResistance();
			(hwSensorReader->newSensorValues_)[SENSOR::WATER_TEMP_C] = wT.get();
			(hwSensorReader->newSensorValues_)[SENSOR::WATER_TEMP_R] = wT.getResistance();

			// Return mutex
			xSemaphoreGive(hwSensorReader->newSensorValuesMutex_);
		}
	}

	vTaskDelete(nullptr);
}

void HwSensorReader::writeSensorsTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	{
		auto hwSensorReader = static_cast<HwSensorReader*>(p_param);

		// Create all isr sensors
		Speed speed;
		Rpm rpm;

		while (true) {
			// Stop if ordered to
			if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(SENSORS_WRITE_INTERVAL_MS)) != 0) {
				break;
			}

			// Get newest sensor values
			const int speedVal = speed.get();
			const int rpmVal = rpm.get();

			// Acquire mutex
			if (xSemaphoreTake(hwSensorReader->newSensorValuesMutex_, pdMS_TO_TICKS(SENSORS_WRITE_INTERVAL_MS / 2)) ==
				pdFALSE) {
				continue;
			}

			// Update sensor values of the ISR sensors
			(hwSensorReader->newSensorValues_)[SENSOR::SPEED] = speedVal;
			(hwSensorReader->newSensorValues_)[SENSOR::RPM] = rpmVal;

			// Update values in the SensorValues object
			auto currData = hwSensorReader->sensorValues_->getWriteAccess();
			if (currData != nullptr) {
				for (auto& entry : hwSensorReader->newSensorValues_) {
					(*currData)[entry.first] = entry.second;
				}

				hwSensorReader->sensorValues_->returnWriteAccess();
			}

			// Return mutex
			xSemaphoreGive(hwSensorReader->newSensorValuesMutex_);
		}
	}

	vTaskDelete(nullptr);
}

void HwSensorReader::indicatorTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	{
		auto hwSensorReader = static_cast<HwSensorReader*>(p_param);

		// Create Sensors
		RightIndicator rIndic;
		rIndic.setTaskToNotifyOnChange(xTaskGetCurrentTaskHandle());
		LeftIndicator lIndic;
		lIndic.setTaskToNotifyOnChange(xTaskGetCurrentTaskHandle());

		uint32_t bits = 0;
		while (true) {
			// Wait for notification
			// b01 = Stop Task
			// b10 = New Indicator Value
			xTaskNotifyWait(0b000, 0b11, &bits, portMAX_DELAY);

			// Should the task stop?
			if (bits & 0b01) {
				break;
			}

			auto rightIndicActive = rIndic.get();
			auto leftIndicActive = lIndic.get();

			if (xSemaphoreTake(hwSensorReader->newSensorValuesMutex_, portMAX_DELAY) == pdFALSE) {
				continue;
			}

			// Update sensor values of the ISR sensors
			(hwSensorReader->newSensorValues_)[SENSOR::RIGHT_INDICATOR] = rightIndicActive;
			(hwSensorReader->newSensorValues_)[SENSOR::LEFT_INDICATOR] = leftIndicActive;

			// Update value in the SensorsValue object
			auto currData = hwSensorReader->sensorValues_->getWriteAccess();
			if (currData != nullptr) {
				(*currData)[SENSOR::RIGHT_INDICATOR] = (hwSensorReader->newSensorValues_)[SENSOR::RIGHT_INDICATOR];
				(*currData)[SENSOR::LEFT_INDICATOR] = (hwSensorReader->newSensorValues_)[SENSOR::LEFT_INDICATOR];

				hwSensorReader->sensorValues_->returnWriteAccess();
			}

			// Return mutex
			xSemaphoreGive(hwSensorReader->newSensorValuesMutex_);

			// Notify the broadcaster, that the indicator states changed
			if (hwSensorReader->indicatorChangedCb_) {
				hwSensorReader->indicatorChangedCb_();
			}
		}
	}

	vTaskDelete(nullptr);
}
