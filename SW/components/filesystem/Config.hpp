#pragma once

// Project includes
#include "Filesystem.hpp"

// C++ includes
#include <string>
#include <vector>

// espidf includes
#include "ArduinoJson.hpp"

class Config
{
public:
	Config() = default;

	Config(Filesystem* p_fs);

	Config(Filesystem* p_fs, const std::string& path);

	~Config();

	bool open(const std::string& path);

	bool open();

	void close();

	ArduinoJson::JsonDocument getReadAccess();

	ArduinoJson::JsonDocument* getWriteAccess();

	void returnWriteAccess();

	bool save();

	void print();

private:
	/*
	 *   Private Functions
	 */
	long getFileSize();

	bool isFileEmpty();

	std::vector<char> readFileContent();

	bool extractJsonFromFile(std::vector<char>& fileContent);

	void startSaveTask();
	void stopSaveTask();

	/*
	 *   Private Variables
	 */
	Filesystem* fs_ = nullptr;

	std::string path_;

	FILE* file_ = nullptr;

	ArduinoJson::JsonDocument json_;

	bool configChanged_ = false;
	SemaphoreHandle_t configMutex_ = nullptr;
	TaskHandle_t saveTask_ = nullptr;
};
