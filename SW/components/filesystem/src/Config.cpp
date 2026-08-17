#include "Config.hpp"

// C++ includes
#include <vector>

/*
 *	constexpr
 */
constexpr auto TAG = "Config";

/*
 *	Public Function Implementations
 */
Config::Config(SystemContext* p_sysCon)
{
	sysCon_ = p_sysCon;
}

bool Config::open(const std::string& path)
{
	path_ = path;

	if(!sysCon_->filesystem->doesFileExist(path_, Filesystem::CONFIG_PARTITION)) {
		ESP_LOGW(TAG, "Couldn't open config file. File %s does not exist", path_.c_str());
		return false;
	}

	file_ = sysCon_->filesystem->openFile(path_, "r", Filesystem::CONFIG_PARTITION);
	if(file_ == nullptr) {
		ESP_LOGW(TAG, "Failed to open config file %s", path_.c_str());
		return false;
	}

	fseek(file_, 0, SEEK_END);
	const long fileSize = ftell(file_);
	fseek(file_, 0, SEEK_SET);
	if (fileSize <= 0) {
		fclose(file_);
		file_ = nullptr;

		ESP_LOGW(TAG, "Failed to read config file %s. File is empty", path_.c_str());
		return false;
	}

	std::vector<char> fileContent(fileSize);
	const size_t bytesRead = fread(fileContent.data(), 1, fileSize, file_);
	if (bytesRead <= 0) {
		fclose(file_);
		file_ = nullptr;

		ESP_LOGW(TAG, "Failed to read config file %s. File is empty", path_.c_str());
		return false;
	}

	fclose(file_);
	file_ = nullptr;

	if (ArduinoJson::deserializeJson(json_, fileContent.data(), bytesRead) != ArduinoJson::DeserializationError::Ok) {
		ESP_LOGW(TAG, "Failed to parse config file %s", path_.c_str());
		return false;
	}

	return true;
}

Config::~Config()
{
	if (file_ != nullptr) {
		if (!json_.isNull()) {
			save();
			json_.clear();
		}

		fclose(file_);
		file_ = nullptr;
	}
}

ArduinoJson::JsonDocument* Config::getJson()
{
	return &json_;
}

bool Config::save()
{
	std::string output = "";
	ArduinoJson::serializeJsonPretty(json_, output);

	if(!sysCon_->filesystem->doesFileExist(path_, Filesystem::CONFIG_PARTITION)) {
		ESP_LOGW(TAG, "Couldn't open config file for saving. File %s does not exist", path_.c_str());
		return false;
	}

	file_ = sysCon_->filesystem->openFile(path_, "w", Filesystem::CONFIG_PARTITION);
	if(file_ == nullptr) {
		ESP_LOGW(TAG, "Failed to open config file %s", path_.c_str());
		return false;
	}

	if (fprintf(file_, "%s", output.c_str()) <= 0) {
		ESP_LOGW(TAG, "Failed to write updated config %s", path_.c_str());
		return false;
	}

	fclose(file_);

	return true;
}
