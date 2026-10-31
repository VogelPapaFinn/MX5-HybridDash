#include "Sensors/SwSensorReader.hpp"

// Project includes
#include "Sensors/SwSensor/BitfieldSensors.hpp"
#include "Sensors/SwSensor/NumericalSensors.hpp"

// espidf includes
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr auto TAG = "SwSensorReader";

constexpr auto TIME_BETWEEN_EACH_SENSOR_READ_MS = 200;

constexpr auto SENSORS_TO_READ = {SENSOR::ECU_COOLANT_C, SENSOR::ECU_COOLANT_V, SENSOR::ECU_INJECTION_MS,
								  SENSOR::ECU_RPM, SENSOR::ECU_SPEED};

/*
 *   Public Function Implementations
 */
SwSensorReader::SwSensorReader(KLine* p_kline, SensorValues* p_sensorValues)
{
	kline_ = p_kline;
	sensorValues_ = p_sensorValues;
}

SwSensorReader::~SwSensorReader() { stop(); }

void SwSensorReader::start() { startReadThread(); }

void SwSensorReader::stop() { stopReadThread(); }

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

	auto swSensorReader = static_cast<SwSensorReader*>(p_param);

	while (true) {
		for (auto& sensorType : SENSORS_TO_READ) {
			// Stop if ordered to
			if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(TIME_BETWEEN_EACH_SENSOR_READ_MS)) != 0) {
				vTaskDelete(nullptr);
			}

			const auto sensor = getNumericalSensor(sensorType);
			KLineMessage message(
				0x22, {static_cast<uint8_t>((sensor.pid >> 8) & 0xFF), static_cast<uint8_t>((sensor.pid >> 0) & 0xFF)});
			swSensorReader->kline_->send(message,
										 [swSensorReader, sensor](KLineMessage& response)
										 {
											 // ERROR
											 if (response.getSid() == 0x7F) {
												 return;
											 }


											 // Check payload length
											 const auto payload = response.getPayload();
											 if (payload.size() != sensor.byteCount) {
												 return;
											 }

											 // Extract raw sensor value
											 uint16_t rawValue = 0;
											 uint8_t i = payload.size();
											 for (const auto& b : response.getPayload()) {
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
	}
}
