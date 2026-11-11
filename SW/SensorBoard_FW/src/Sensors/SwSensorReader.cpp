#include "Sensors/SwSensorReader.hpp"

// Project includes
#include "Sensors/SwSensor/BitfieldSensors.hpp"
#include "Sensors/SwSensor/NumericalSensors.hpp"

// C++ includes
#include <algorithm>

// espidf includes
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr auto TAG = "SwSensorReader";

constexpr auto TIME_BETWEEN_EACH_SENSOR_READ_MS = 200;

/*
 *   Public Function Implementations
 */
SwSensorReader::SwSensorReader(KLine* p_kline, SensorValues* p_sensorValues)
{
	kline_ = p_kline;
	sensorValues_ = p_sensorValues;

	sensorsToReadMutex_ = xSemaphoreCreateMutex();
}

SwSensorReader::~SwSensorReader() { stop(); }

void SwSensorReader::start() { startReadThread(); }

void SwSensorReader::stop() { stopReadThread(); }

void SwSensorReader::readOnce()
{
	start();
	stop();
}

void SwSensorReader::setSensorsToRead(std::vector<SENSOR> sensors)
{
	xSemaphoreTake(sensorsToReadMutex_, portMAX_DELAY);

	sensorsToRead_ = std::move(sensors);

	xSemaphoreGetMutexHolder(sensorsToReadMutex_);
}

void SwSensorReader::addSensorToRead(SENSOR sensor)
{
	// Ignore if already being read
	if (std::find(sensorsToRead_.begin(), sensorsToRead_.end(), sensor) != sensorsToRead_.end()) {
		return;
	}

	xSemaphoreTake(sensorsToReadMutex_, portMAX_DELAY);

	sensorsToRead_.push_back(sensor);

	xSemaphoreGetMutexHolder(sensorsToReadMutex_);
}

void SwSensorReader::removeSensorFromRead(SENSOR sensor)
{
	// Ignore if not being read
	const auto it = std::find(sensorsToRead_.begin(), sensorsToRead_.end(), sensor);
	if (it == sensorsToRead_.end()) {
		return;
	}

	xSemaphoreTake(sensorsToReadMutex_, portMAX_DELAY);

	sensorsToRead_.erase(it);

	xSemaphoreGetMutexHolder(sensorsToReadMutex_);
}

void SwSensorReader::stopReadingAllSensors()
{
	if (sensorsToRead_.empty()) {
		return;
	}

	xSemaphoreTake(sensorsToReadMutex_, portMAX_DELAY);

	sensorsToRead_.clear();

	xSemaphoreGetMutexHolder(sensorsToReadMutex_);
}

/*
 *   Private Function Implementations
 */
void SwSensorReader::startReadThread()
{
	if (readSensorsTask_ != nullptr) {
		return;
	}

	if (xTaskCreate(readSensors, "ReadEcuSensors", 4096, this, 2, &readSensorsTask_) == pdPASS) {
		// Success
		return;
	}

	// Fail
	readSensorsTask_ = nullptr;
	ESP_LOGW(TAG, "Failed to start task which reads some ECU sensors");
	return;
}

void SwSensorReader::stopReadThread()
{
	if (readSensorsTask_ == nullptr) {
		return;
	}

	xTaskNotifyGive(readSensorsTask_);
	readSensorsTask_ = nullptr;
}

/*
 *   Private Function Implementations
 */
void SwSensorReader::readSensors(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	{
		auto swSensorReader = static_cast<SwSensorReader*>(p_param);

		bool stop = false;
		while (true) {
			xSemaphoreTake(swSensorReader->sensorsToReadMutex_, portMAX_DELAY);

			for (auto& sensorType : swSensorReader->sensorsToRead_) {
				// Stop if ordered to
				if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(TIME_BETWEEN_EACH_SENSOR_READ_MS)) != 0) {
					stop = true;
					break;
				}

				const auto sensor = getNumericalSensor(sensorType);
				KLineMessage message(
					0x22,
					{static_cast<uint8_t>((sensor.pid >> 8) & 0xFF), static_cast<uint8_t>((sensor.pid >> 0) & 0xFF)});
				swSensorReader->kline_->send(message,
											 [swSensorReader, sensor](KLineMessage& response)
											 {
												 // ERROR
												 if (response.getSid() == 0x7F) {
													 return;
												 }

												 // Check PID
												 auto payload = response.getPayload();
												 if (payload.size() <= 2 || payload.at(0) != (sensor.pid >> 8) ||
													 payload.at(1) != (sensor.pid & 0xFF)) {
													 return;
												 }

												 // Remove the pid
												 payload.erase(payload.begin(), payload.begin() + 2);

												 // Check payload length
												 if (payload.size() != sensor.byteCount) {
													 return;
												 }

												 // Extract raw sensor value
												 uint16_t rawValue = 0;
												 uint8_t i = payload.size();
												 for (const auto& b : payload) {
													 rawValue += (b << ((i - 1) * 8));

													 i--;
												 }

												 // Get logical value
												 const double value = sensor.convertFunc(rawValue);

												 // Save value
												 auto sensorValues = swSensorReader->sensorValues_->getWriteAccess();
												 (*sensorValues)[sensor.id] = value;
												 swSensorReader->sensorValues_->returnWriteAccess();
											 });
			}

			xSemaphoreGive(swSensorReader->sensorsToReadMutex_);

			if (stop) {
				break;
			}
		}
	}

	vTaskDelete(nullptr);
}
