#pragma once

// Project includes
#include "Config.hpp"
#include "SensorsData.hpp"

// C++ includes
#include <string>

class SensorLogger
{
public:
	SensorLogger(Filesystem* p_fs, Config* p_config, SensorsData* p_sensorsData);

private:
	/*
	 *   Private Functions
	 */
	unsigned int getFileIndex();

	void printHeader();

	void startLogTask();

	/*
	 *   Private Variables
	 */
	Filesystem* fs_ = nullptr;
	Config* config_ = nullptr;
	SensorsData* sensorsData_ = nullptr;

	std::string csvPath_;
	FILE* csvFile_ = nullptr;

	TaskHandle_t taskHandle_ = nullptr;
};
