#include "Sensors/SensorBroadcaster.hpp"

// espidf includes
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr auto TAG = "SensorBroadcaster";

constexpr unsigned int BROADCAST_INTERVALL_MS = 100;

/*
 *   Public Function Implementations
 */
SensorBroadcaster::SensorBroadcaster(Can* p_can, SensorValues* p_sensorValues)
{
	can_ = p_can;
	sensorValues_ = p_sensorValues;
}

void SensorBroadcaster::start()
{
	if (broadcastTaskHandle_ != nullptr) {
		return;
	}

	if (xTaskCreate(broadcastTask, "BroadcastTask", 4096, this, 2, &broadcastTaskHandle_) == pdPASS) {
		// Success
		return;
	}

	ESP_LOGW(TAG, "Failed to create broadcast task, aborting");
	broadcastTaskHandle_ = nullptr;
}

void SensorBroadcaster::stop()
{
	if (broadcastTaskHandle_ == nullptr) {
		return;
	}

	xTaskNotifyGive(broadcastTaskHandle_);
	broadcastTaskHandle_ = nullptr;
}

/*
 *   Private Task Function Implementations
 */
void SensorBroadcaster::broadcastTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto sensorBroadcaster = static_cast<SensorBroadcaster*>(p_param);

	while (true) {
		// Stop if ordered to
		if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BROADCAST_INTERVALL_MS)) != 0) {
			vTaskDelete(nullptr);
		}

		const auto sensorData = sensorBroadcaster->sensorValues_->getSnapshot();

		// Build header
		Can::Frame_t frame;
		frame.sender = CAN_MASTER_ID;
		frame.target = CAN_BROADCAST_ID;
		frame.group = CanFrameGroups::GROUP::SENSOR;
		frame.function = CanFrameGroups::FUNC::BROADCAST_DATA;
		frame.dataLengthCode = 8;
		frame.answer = false;

		// Build payload
		frame.data[0] = sensorData.at(SENSOR::FUEL_LEVEL_P);
		frame.data[1] = sensorData.at(SENSOR::OIL_PRESSURE_B);
		frame.data[2] = sensorData.at(SENSOR::WATER_TEMP_C);
		frame.data[3] = (sensorData.at(SENSOR::RPM) >> 8) & 0xFF;
		frame.data[4] = (sensorData.at(SENSOR::RPM) >> 0) & 0xFF;
		frame.data[5] = sensorData.at(SENSOR::SPEED);
		frame.data[6] = sensorData.at(SENSOR::LEFT_INDICATOR);
		frame.data[7] = sensorData.at(SENSOR::RIGHT_INDICATOR);

		// Transmit frame
		sensorBroadcaster->can_->queueFrame(frame);
	}
}
