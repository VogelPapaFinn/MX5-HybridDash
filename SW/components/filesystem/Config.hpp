#pragma once

// Project includes
#include "Filesystem.hpp"

// C++ includes
#include <string>

// espidf includes
#include "ArduinoJson.hpp"

class Config
{
public:
	Config(SystemContext* p_sysCon);

	~Config();

	bool open(const std::string& path);

	ArduinoJson::JsonDocument* getJson();

	bool save();

private:
	SystemContext* sysCon_ = nullptr;

	std::string path_;

	FILE* file_ = nullptr;

	ArduinoJson::JsonDocument json_;
};
