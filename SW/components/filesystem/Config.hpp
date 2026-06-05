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
	Config(const std::string& path);

	~Config();

	ArduinoJson::JsonDocument* getJson();

	bool save();

private:
	Filesystem* filesystem_ = nullptr;

	std::string path_;

	FILE* file_ = nullptr;

	ArduinoJson::JsonDocument json_;
};
