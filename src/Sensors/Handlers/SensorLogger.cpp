#include "Sensors/Handlers/SensorLogger.hpp"

// Project includes
#include "Sensors/Handlers/SensorsData.hpp"
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
 *   Private struct
 */
struct TaskStuff
{
	FILE* csvFile = nullptr;
	SensorsData* sensorsData = nullptr;
};

/*
 *	Task Functions
 */
static void logTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto taskStuff = static_cast<TaskStuff*>(p_param);

	unsigned int counter = 0;
	while (true) {
		vTaskDelay(pdMS_TO_TICKS(1000));

		// Get snapshot
		auto sensorData = taskStuff->sensorsData->getSnapshot();

		// Print data of all sensors
		for (const auto& sensor : SENSORS_TO_LOG) {
			fprintf(taskStuff->csvFile, "%d,", sensorData.at(sensor));
		}
		fprintf(taskStuff->csvFile, ";\n");

		counter++;

		// Only flush to the actual file every 10th time
		if (counter < 10) {
			continue;
		}

		fflush(taskStuff->csvFile);
		fsync(fileno(taskStuff->csvFile));
		counter = 0;
	}
}

/*
 *	Public Function Implementations
 */
SensorLogger::SensorLogger(Filesystem* p_fs, Config* p_config, SensorsData* p_sensorsData)
{
	fs_ = p_fs;
	config_ = p_config;
	sensorsData_ = p_sensorsData;

	// Build file name
	csvPath_ = "SensorData_x.csv";
	csvPath_.replace(csvPath_.find('x'), csvPath_.find('x') + 1, std::to_string(getFileIndex()));

	// Create file
	fs_->createFile(csvPath_, Filesystem::SD_CARD);

	// Open the file
	csvFile_ = fs_->openFile(csvPath_, "w", Filesystem::SD_CARD);

	printHeader();

	startLogTask();
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

void SensorLogger::printHeader()
{
	for (const auto& sensor : SENSORS_TO_LOG) {
		fprintf(csvFile_, "%s,", g_SENSOR_NAME.at(sensor).c_str());
	}
	fprintf(csvFile_, ";\n");

	fflush(csvFile_);
	fsync(fileno(csvFile_));
}

void SensorLogger::startLogTask()
{
	TaskStuff taskStuff = {.csvFile = csvFile_, .sensorsData = sensorsData_};

	if (xTaskCreate(logTask, "WriteSensors", 4096, &taskStuff, 2, &taskHandle_) == pdPASS) {
		// Success
		return;
	}

	// Fail
	taskHandle_ = nullptr;
	ESP_LOGW(TAG, "Failed to start task which logs the sensor values");
	return;
}
