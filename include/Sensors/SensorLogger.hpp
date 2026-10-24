#pragma once

// Project includes
#include "Config.hpp"
#include "SensorValues.hpp"

// C++ includes
#include <string>

class SensorLogger
{
public:
	SensorLogger(Filesystem* p_fs, Config* p_config, SensorValues* p_sensorValues);

	~SensorLogger();

	void start();

	void stop();

private:
	/*
	 *   Private Functions
	 */
	void createAndOpenFile();

	int getFileIndex();

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
	Config* config_ = nullptr;
	SensorValues* sensorValues_ = nullptr;

	std::string csvPath_;
	FILE* csvFile_ = nullptr;

	TaskHandle_t taskHandle_ = nullptr;
};
