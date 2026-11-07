#include "Sensors/SensorLogger.hpp"

// Project includes
#include "Sensors/SensorValues.hpp"
#include "Sensors/Sensors.hpp"

/*
 *	constexpr
 */
constexpr auto TAG = "SensorLogger";
constexpr auto SENSORS_TO_LOG = {SENSOR::FUEL_LEVEL_R,
								 SENSOR::FUEL_LEVEL_P,
								 SENSOR::OIL_PRESSURE_R,
								 SENSOR::OIL_PRESSURE_B,
								 SENSOR::WATER_TEMP_R,
								 SENSOR::WATER_TEMP_C,
								 SENSOR::SPEED,
								 SENSOR::RPM,
								 SENSOR::ECU_COOLANT_C,
								 SENSOR::ECU_COOLANT_V,
								 SENSOR::ECU_INJECTION_MS,
								 SENSOR::ECU_RPM,
								 SENSOR::ECU_SPEED};

constexpr auto NVS_PARTITION = "SensorLogger";
constexpr auto NVS_FILE_INDEX = "Index";

/*
 *	Public Function Implementations
 */
SensorLogger::SensorLogger(Filesystem* p_fs, SensorValues* p_sensorValues)
{
	fs_ = p_fs;
	sensorValues_ = p_sensorValues;

	nvs_open(NVS_PARTITION, NVS_READWRITE, &nvsHandle_);
}

SensorLogger::~SensorLogger()
{
	stop();

	if (csvFile_ != nullptr) {
		fclose(csvFile_);
		csvFile_ = nullptr;
	}

	nvs_close(nvsHandle_);
}

/*
 *	Private Function Implementations
 */
bool SensorLogger::createAndOpenFile()
{
	// Build file name
	csvPath_ = "SensorData_x.csv";
	const auto fileIndex = getFileIndex();
	csvPath_.replace(csvPath_.find('x'), 1, std::to_string(fileIndex));
	ESP_LOGI(TAG, "Starting new log file with index %d", fileIndex);

	increaseFileIndex();

	// Create file
	fs_->createFile(csvPath_, Filesystem::SD_CARD);

	// Open the file
	csvFile_ = fs_->openFile(csvPath_, "w", Filesystem::SD_CARD);
	if (csvFile_ == nullptr) {
		return false;
	}

	printHeader();

	return true;
}

uint32_t SensorLogger::getFileIndex()
{
	uint32_t value = 0;

	nvs_get_u32(nvsHandle_, NVS_FILE_INDEX, &value);

	return value;
}

void SensorLogger::increaseFileIndex() { nvs_set_u32(nvsHandle_, NVS_FILE_INDEX, getFileIndex()); }

void SensorLogger::printHeader()
{
	std::string row = "";
	for (const auto& sensor : SENSORS_TO_LOG) {
		row += g_SENSOR_NAME.at(sensor) + ",";
	}
	fprintf(csvFile_, "%s;\n", row.c_str());

	ESP_LOGI(TAG, "Log Header: %s", row.c_str());

	fflush(csvFile_);
	fsync(fileno(csvFile_));
}

void SensorLogger::start()
{
	if (!createAndOpenFile()) {
		ESP_LOGW(TAG, "Failed to create and open new log file");
		return;
	}

	if (xTaskCreate(logTask, "LogTask", 4096, this, 2, &taskHandle_) == pdPASS) {
		// Success
		return;
	}

	// Fail
	taskHandle_ = nullptr;
	ESP_LOGW(TAG, "Failed to start task which logs the sensor values");
	return;
}

void SensorLogger::stop()
{
	if (taskHandle_ == nullptr) {
		return;
	}

	xTaskNotifyGive(taskHandle_);
	taskHandle_ = nullptr;
	return;
}

/*
 *	Private Task Functions Implementations
 */
void SensorLogger::logTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	{
		auto sensorLogger = static_cast<SensorLogger*>(p_param);

		unsigned int counter = 0;
		while (true) {
			// Stop if ordered to
			if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000)) != 0) {
				fflush(sensorLogger->csvFile_);
				fsync(fileno(sensorLogger->csvFile_));
				break;
			}

			// Get snapshot
			auto sensorData = sensorLogger->sensorValues_->getSnapshot();

			// Print data of all sensors
			std::string row = "";
			for (const auto& sensor : SENSORS_TO_LOG) {
				row += std::to_string(sensorData.at(sensor)) + ",";
			}
			fprintf(sensorLogger->csvFile_, "%s;\n", row.c_str());

			counter++;

			// ESP_LOGI(TAG, "Added line to log: %s", row.c_str());

			// Only flush to the actual file every 10th time
			if (counter < 10) {
				continue;
			}

			fflush(sensorLogger->csvFile_);
			fsync(fileno(sensorLogger->csvFile_));
			counter = 0;
		}
	}

	vTaskDelete(nullptr);
}
