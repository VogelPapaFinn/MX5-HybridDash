#pragma once

// Project includes
#include "Config.hpp"
#include "SensorValues.hpp"

// C++ includes
#include <string>

// espidf includes
#include "nvs_flash.h"

class SensorLogger
{
public:
	SensorLogger(Filesystem* p_fs, SensorValues* p_sensorValues);

	~SensorLogger();

	void start();

	void stop();

private:
	/*
	 *   Private Functions
	 */
	bool createAndOpenFile();

	uint32_t getFileIndex();

	void increaseFileIndex();

	void printHeader();

	/*
	 *	Private Task Functions
	 */
	static void logTask(void* p_param);

	/*
	 *   Private Variables
	 */
	Filesystem* fs_ = nullptr;
	SensorValues* sensorValues_ = nullptr;

	nvs_handle_t nvsHandle_ = 0;

	std::string csvPath_;
	FILE* csvFile_ = nullptr;

	TaskHandle_t taskHandle_ = nullptr;
	TimerHandle_t increaseFileIndexTimer_ = nullptr;
};
