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

void SwSensorReader::setSensorsToRead(std::vector<SENSOR> sensors)
{
	xSemaphoreTake(sensorsToReadMutex_, portMAX_DELAY);

	sensorsToRead_ = std::move(sensors);

	xSemaphoreGive(sensorsToReadMutex_);
}

void SwSensorReader::addSensorToRead(SENSOR sensor)
{
	xSemaphoreTake(sensorsToReadMutex_, portMAX_DELAY);

	// Ignore if already being read
	if (std::find(sensorsToRead_.begin(), sensorsToRead_.end(), sensor) == sensorsToRead_.end()) {
		sensorsToRead_.push_back(sensor);
	}

	xSemaphoreGive(sensorsToReadMutex_);
}

void SwSensorReader::removeSensorFromRead(SENSOR sensor)
{
	xSemaphoreTake(sensorsToReadMutex_, portMAX_DELAY);

	// Ignore if not being read
	const auto it = std::find(sensorsToRead_.begin(), sensorsToRead_.end(), sensor);
	if (it != sensorsToRead_.end()) {
		sensorsToRead_.erase(it);
	}

	xSemaphoreGive(sensorsToReadMutex_);
}

void SwSensorReader::stopReadingAllSensors()
{
	xSemaphoreTake(sensorsToReadMutex_, portMAX_DELAY);

	sensorsToRead_.clear();

	xSemaphoreGive(sensorsToReadMutex_);
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

	// Wait until the task has really finished
	while (sensorsReadTaskRunning_) {
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

/*
 *   Private Function Implementations
 */
void SwSensorReader::readSensors(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto swSensorReader = static_cast<SwSensorReader*>(p_param);

	{
		swSensorReader->sensorsReadTaskRunning_ = true;

		bool stop = false;
		while (true) {
			// Acquire copy of sensors to read
			xSemaphoreTake(swSensorReader->sensorsToReadMutex_, portMAX_DELAY);
			const auto sensorsToRead = swSensorReader->sensorsToRead_;
			xSemaphoreGive(swSensorReader->sensorsToReadMutex_);

			// Wait if vector is empty
			if (sensorsToRead.empty()) {
				if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000)) != 0) {
					break;
				}
				continue;
			}

			for (auto& sensorType : sensorsToRead) {
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

			if (stop) {
				break;
			}
		}
	}

	swSensorReader->sensorsReadTaskRunning_ = false;
	vTaskDelete(nullptr);
}
