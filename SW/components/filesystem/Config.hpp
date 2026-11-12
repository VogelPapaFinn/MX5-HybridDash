#pragma once

// Project includes
#include "Filesystem.hpp"

// C++ includes
#include <string>
#include <vector>

// espidf includes
#include "ArduinoJson.hpp"
#include "freertos/FreeRTOS.h"

class Config
{
public:
	Config() = default;

	Config(Filesystem* p_fs, uint8_t fsLocation);

	Config(Filesystem* p_fs, uint8_t fsLocation, const std::string& path);

	~Config();

	bool open(const std::string& path);

	bool open();

	void close();

	ArduinoJson::JsonDocument getReadAccess();

	ArduinoJson::JsonDocument* getWriteAccess();

	void returnWriteAccess();

	void print();

private:
	/*
	 *   Private Functions
	 */
	bool save();

	long getFileSize();

	bool isFileEmpty();

	std::vector<char> readFileContent();

	bool extractJsonFromFile(std::vector<char>& fileContent);

	/*
	 *   Private Variables
	 */
	Filesystem* fs_ = nullptr;

	uint8_t fsLocation_ = 0;
	std::string path_;

	FILE* file_ = nullptr;

	ArduinoJson::JsonDocument json_;
	SemaphoreHandle_t configMutex_ = nullptr;
};
