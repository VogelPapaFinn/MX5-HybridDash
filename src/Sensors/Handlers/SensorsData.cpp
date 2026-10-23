#include "Sensors/Handlers/SensorsData.hpp"

// espidf includes
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr auto TAG = "SensorsData";
constexpr unsigned int BROADCAST_INTERVALL_MS = 100;

/*
 *   Private struct
 */
struct TaskStuff
{
	Can* can = nullptr;
	SensorsData* sensorsData = nullptr;
};

/*
 *	Task Functions
 */
static void broadcastTask(void* p_param)
{
	if (p_param == nullptr) {
		return;
	}

	auto taskStuff = static_cast<TaskStuff*>(p_param);

	while (true) {
		vTaskDelay(pdMS_TO_TICKS(BROADCAST_INTERVALL_MS));

		const auto sensorData = taskStuff->sensorsData->getSnapshot();

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
		taskStuff->can->queueFrame(frame);
	}
}

/*
 *   Public Function Implementations
 */

SensorsData::SensorsData(Can* p_can)
{
	can_ = p_can;

	writeMutex_ = xSemaphoreCreateMutex();
}

std::unordered_map<SENSOR, int> SensorsData::getSnapshot() { return sensorsData_; }

std::unordered_map<SENSOR, int>* SensorsData::getWriteAccess()
{
	if (xSemaphoreTake(writeMutex_, pdMS_TO_TICKS(MAX_WAITING_TIME_FOR_WRITE_ACCESS_MS))) {
		return &sensorsData_;
	}

	return nullptr;
}

void SensorsData::returnWriteAccess() { xSemaphoreGive(writeMutex_); }

void SensorsData::startBroadcasting()
{
	if (broadcastTaskHandle_ != nullptr) {
		return;
	}

	if (xTaskCreate(broadcastTask, "BroadcastTask", 4096, can_, 2, &broadcastTaskHandle_) == pdPASS) {
		// Success
		return;
	}

	ESP_LOGW(TAG, "Failed to create broadcast task, aborting");
	broadcastTaskHandle_ = nullptr;
}

void SensorsData::stopBroadcasting()
{
	if (broadcastTaskHandle_ == nullptr) {
		return;
	}

	vTaskDelete(broadcastTaskHandle_);
	broadcastTaskHandle_ = nullptr;
}
