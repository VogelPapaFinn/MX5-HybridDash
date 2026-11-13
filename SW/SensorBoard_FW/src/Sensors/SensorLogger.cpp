#include "Sensors/SensorLogger.hpp"

// Project includes
#include "FilesystemLocations.hpp"
#include "Sensors/SensorValues.hpp"
#include "Sensors/Sensors.hpp"

// espidf includes
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "SensorLogger";
constexpr auto SENSORS_TO_LOG = {SENSOR::FUEL_LEVEL_R,
								 SENSOR::FUEL_LEVEL_MV,
								 SENSOR::FUEL_LEVEL_P,
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

constexpr auto NVS_NAMESPACE = "SensorLogger";
constexpr auto NVS_FILE_INDEX = "Index";

constexpr auto SAVE_INTERVAL_S = 30;

/*
 *	Public Function Implementations
 */
SensorLogger::SensorLogger(Filesystem* p_fs, SensorValues* p_sensorValues)
{
	fs_ = p_fs;
	sensorValues_ = p_sensorValues;

	if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvsHandle_) != ESP_OK) {
		ESP_LOGW(TAG, "Failed to open NVS, this may lead to logs being overwritten!");
	}

	moveAllLogsToSdCard();

	// DEBUGGING PURPOSE
	// nvs_set_u32(nvsHandle_, NVS_FILE_INDEX, 0);
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

	if (increaseFileIndexTimer_ != nullptr) {
		xTimerStop(increaseFileIndexTimer_, 0);
		xTimerDelete(increaseFileIndexTimer_, 0);
		increaseFileIndexTimer_ = nullptr;
	}

	xTaskNotifyGive(taskHandle_);
	taskHandle_ = nullptr;

	return;
}


/*
 *	Private Function Implementations
 */
void SensorLogger::moveAllLogsToSdCard()
{
	auto fileNames = fs_->getAllFileNamesInDirectory("", "*.csv", FsLocation::LOG_PARTITION);

	for (unsigned int i = 0; i < fileNames.size(); i++) {
		FILE* src = fs_->openFile(fileNames[i], "r", FsLocation::LOG_PARTITION);

		fs_->createFile(fileNames[i], FsLocation::SD_CARD);
		FILE* tar = fs_->openFile(fileNames[i], "w", FsLocation::SD_CARD);

		if (src == nullptr || tar == nullptr) {
			ESP_LOGW(TAG, "Failed to move file %s from internal log partition to SD Card!", fileNames[i].c_str());

			if (src != nullptr) {
				fclose(src);
			}
			if (tar != nullptr) {
				fclose(tar);
			}

			fileNames.erase(fileNames.begin() + i);
			i--;

			continue;
		}

		static char buf[4096];
		size_t bytesRead;
		while ((bytesRead = fread(buf, 1, sizeof(buf), src)) > 0) {
			if (fwrite(buf, 1, bytesRead, tar) != bytesRead) {
				ESP_LOGW(TAG, "Failed to copy contents of file %s from internal log partition to SD Card!",
						 fileNames[i].c_str());

				fileNames.erase(fileNames.begin() + i);
				i--;

				break;
			}
		}

		fclose(src);
		fclose(tar);
	}

	for (const auto& fileName : fileNames) {
		fs_->deleteFile(fileName, FsLocation::LOG_PARTITION);
	}
}

bool SensorLogger::createAndOpenFile()
{
	// Get index
	const auto index = getFileIndex();
	ESP_LOGI(TAG, "Using file index %d for the new .csv", index);
	increaseFileIndex();

	// Build file name
	csvPath_ = "SensorData_x.csv";
	csvPath_.replace(csvPath_.find('x'), 1, std::to_string(index));

	// Create file
	fs_->createFile(csvPath_, FsLocation::LOG_PARTITION);

	// Open the file
	csvFile_ = fs_->openFile(csvPath_, "w", FsLocation::LOG_PARTITION);
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

void SensorLogger::increaseFileIndex()
{
	if (increaseFileIndexTimer_ != nullptr) {
		return;
	}

	// Save after 4 Minutes, so a short engine start to move the car doesnt produce a whole file, which could clutter
	// the filesystem
	increaseFileIndexTimer_ =
		xTimerCreate("IncreaseSensorLoggerFileIndex", pdMS_TO_TICKS(60000 * 4), pdFALSE, this,
					 [](TimerHandle_t p_timer)
					 {
						 auto sensorLogger = static_cast<SensorLogger*>(pvTimerGetTimerID(p_timer));

						 const auto newIndex = sensorLogger->getFileIndex() + 1;


						 if (auto result = nvs_set_u32(sensorLogger->nvsHandle_, NVS_FILE_INDEX, newIndex) != ESP_OK) {
							 ESP_LOGW(TAG, "nvs_set failed with %s", esp_err_to_name(result));
						 }

						 if (auto result = nvs_commit(sensorLogger->nvsHandle_) != ESP_OK) {
							 ESP_LOGW(TAG, "nvs_commit failed with %s", esp_err_to_name(result));
						 }

						 ESP_LOGI(TAG, "Increased %s to %d", NVS_FILE_INDEX, newIndex);
					 });

	xTimerStart(increaseFileIndexTimer_, 0);
}

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

			// ESP_LOGI(TAG, "%s", row.c_str());

			counter++;

			// Only flush to the actual file every x Seconds
			if (counter < SAVE_INTERVAL_S) {
				continue;
			}

			fflush(sensorLogger->csvFile_);
			fsync(fileno(sensorLogger->csvFile_));
			counter = 0;
		}

		if (sensorLogger->csvFile_ != nullptr) {
			fclose(sensorLogger->csvFile_);
			sensorLogger->csvFile_ = nullptr;
		}
	}

	vTaskDelete(nullptr);
}
