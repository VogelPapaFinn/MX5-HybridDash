#include "Sensors/SensorLogger.hpp"

// Project includes
#include "Sensors/SensorValues.hpp"
#include "Sensors/Sensors.hpp"

/*
 *	constexpr
 */
constexpr auto TAG = "SensorLogger";
constexpr auto SENSORS_TO_LOG = {
	SENSOR::FUEL_LEVEL_R, SENSOR::FUEL_LEVEL_P,	  SENSOR::OIL_PRESSURE_R,  SENSOR::OIL_PRESSURE_B, SENSOR::WATER_TEMP_R,
	SENSOR::WATER_TEMP_C, SENSOR::LEFT_INDICATOR, SENSOR::RIGHT_INDICATOR, SENSOR::SPEED,		   SENSOR::RPM,
};

/*
 *	Public Function Implementations
 */
SensorLogger::SensorLogger(Filesystem* p_fs, Config* p_config, SensorValues* p_sensorValues)
{
	fs_ = p_fs;
	config_ = p_config;
	sensorValues_ = p_sensorValues;

	// Build file name
	csvPath_ = "SensorData_x.csv";
	csvPath_.replace(csvPath_.find('x'), 1, std::to_string(getFileIndex()));

	increaseFileIndex();

	// Create file
	fs_->createFile(csvPath_, Filesystem::SD_CARD);

	// Open the file
	csvFile_ = fs_->openFile(csvPath_, "w", Filesystem::SD_CARD);
}

SensorLogger::~SensorLogger()
{
	stop();

	if (csvFile_ != nullptr) {
		fclose(csvFile_);
		csvFile_ = nullptr;
	}
}

/*
 *	Private Function Implementations
 */
int SensorLogger::getFileIndex()
{
	auto json = config_->getReadAccess();

	if (json["SensorDataFileIndex"].is<int>()) {
		return json["SensorDataFileIndex"].as<int>();
	}

	return 0;
}

void SensorLogger::increaseFileIndex()
{
	auto json = config_->getWriteAccess();
	if (json == nullptr) {
		return;
	}

	(*json)["SensorDataFileIndex"] = (*json)["SensorDataFileIndex"].as<int>() + 1;

	config_->returnWriteAccess();
}

void SensorLogger::printHeader()
{
	for (const auto& sensor : SENSORS_TO_LOG) {
		fprintf(csvFile_, "%s,", g_SENSOR_NAME.at(sensor).c_str());
	}
	fprintf(csvFile_, ";\n");

	fflush(csvFile_);
	fsync(fileno(csvFile_));
}

void SensorLogger::start()
{
	printHeader();

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

	auto sensorLogger = static_cast<SensorLogger*>(p_param);

	unsigned int counter = 0;
	while (true) {
		// Stop if ordered to
		if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000)) != 0) {
			fflush(sensorLogger->csvFile_);
			fsync(fileno(sensorLogger->csvFile_));
			vTaskDelete(nullptr);
		}

		// Get snapshot
		auto sensorData = sensorLogger->sensorValues_->getSnapshot();

		// Print data of all sensors
		for (const auto& sensor : SENSORS_TO_LOG) {
			fprintf(sensorLogger->csvFile_, "%d,", sensorData.at(sensor));
		}
		fprintf(sensorLogger->csvFile_, ";\n");

		counter++;

		// Only flush to the actual file every 10th time
		if (counter < 10) {
			continue;
		}

		fflush(sensorLogger->csvFile_);
		fsync(fileno(sensorLogger->csvFile_));
		counter = 0;
	}
}
