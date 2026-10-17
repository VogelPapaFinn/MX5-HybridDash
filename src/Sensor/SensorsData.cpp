#include "Sensor/SensorsData.hpp"

/*
 *   Static Member Variables
 */
SensorsData* SensorsData::g_instance = nullptr;

/*
 *   Public Function Implementations
 */
SensorsData* SensorsData::get()
{
	if (g_instance == nullptr) {
		g_instance = new SensorsData();
	}
	return g_instance;
}

std::unordered_map<HW_SENSOR, unsigned int> SensorsData::getSnapshot() { return sensorsData_; }

std::unordered_map<HW_SENSOR, unsigned int>* SensorsData::getWriteAccess()
{
	if (xSemaphoreTake(writeMutex_, pdMS_TO_TICKS(MAX_WAITING_TIME_FOR_WRITE_ACCESS_MS))) {
		return &sensorsData_;
	}

	return nullptr;
}

void SensorsData::returnWriteAccess() { xSemaphoreGive(writeMutex_); }

/*
 *   Private Function Implemenentations
 */
SensorsData::SensorsData() { writeMutex_ = xSemaphoreCreateMutex(); }
