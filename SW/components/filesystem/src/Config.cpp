#include "Config.hpp"

// C++ includes
#include <vector>

/*
 *	constexpr
 */
constexpr auto TAG = "Config";
constexpr auto GET_MUTEX_TIMEOUT_MS = 100;

/*
 *	Private Task Struct
 */
struct TaskStuff
{
	bool* configChanged = nullptr;
};

/*
 *	Task Functions
 */
static void saveConfigTask(void* p_param)
{
	if (p_param == nullptr) {
		vTaskDelete(nullptr);
	}

	auto config = static_cast<Config*>(p_param);

	while (true) {
		vTaskDelay(pdMS_TO_TICKS(60000));

		config->save();
	}
}

/*
 *	Public Function Implementations
 */
Config::Config(Filesystem* p_fs, const std::string& path)
{
	fs_ = p_fs;
	path_ = path;
}


Config::~Config()
{
	close();

	if (file_ == nullptr) {
		return;
	}

	if (!json_.isNull()) {
		save();
		json_.clear();
	}

	fclose(file_);
	file_ = nullptr;
}

bool Config::open()
{
	// Create file if necessary
	if (!fs_->doesFileExist(path_, Filesystem::CONFIG_PARTITION)) {
		ESP_LOGW(TAG, "No existing config file found: %s", path_.c_str());
		return false;
	}

	// Open file to read
	file_ = fs_->openFile(path_, "r", Filesystem::CONFIG_PARTITION);
	if (file_ == nullptr) {
		ESP_LOGW(TAG, "Failed to open config file %s", path_.c_str());
		return false;
	}

	// Check if file is empty
	if (isFileEmpty()) {
		fclose(file_);
		file_ = nullptr;
		return false;
	}

	// Read content of file
	std::vector<char> fileContent = readFileContent();

	fclose(file_);
	file_ = nullptr;

	if (!extractJsonFromFile(fileContent)) {
		return false;
	}

	// Create mutex for read/write access to the config
	configMutex_ = xSemaphoreCreateMutex();

	startSaveTask();

	return true;
}

void Config::close()
{
	save();

	stopSaveTask();

	if (configMutex_ != nullptr) {
		vSemaphoreDelete(configMutex_);
	}
}

ArduinoJson::JsonDocument Config::getReadAccess() { return json_; }

ArduinoJson::JsonDocument* Config::getWriteAccess()
{
	if (xSemaphoreTake(configMutex_, pdMS_TO_TICKS(GET_MUTEX_TIMEOUT_MS)) == pdFALSE) {
		return nullptr;
	}

	configChanged_ = true;

	return &json_;
}

void Config::returnWriteAccess() { xSemaphoreGive(configMutex_); }

bool Config::save()
{
	if (!configChanged_) {
		return true;
	}

	std::string output = "";
	ArduinoJson::serializeJsonPretty(json_, output);

	// Create file if it does not exist
	if (!fs_->doesFileExist(path_, Filesystem::CONFIG_PARTITION)) {
		fs_->createFile(path_, Filesystem::CONFIG_PARTITION);
	}

	// Open the file in write mode
	file_ = fs_->openFile(path_, "w", Filesystem::CONFIG_PARTITION);
	if (file_ == nullptr) {
		ESP_LOGW(TAG, "Failed to open config file %s", path_.c_str());
		return false;
	}

	// Print json to the file
	if (fprintf(file_, "%s", output.c_str()) <= 0) {
		ESP_LOGW(TAG, "Failed to write updated config %s", path_.c_str());

		fclose(file_);

		return false;
	}

	fflush(file_);
	fsync(fileno(file_));
	fclose(file_);
	file_ = nullptr;

	configChanged_ = false;

	return true;
}

void Config::print()
{
	std::string json;
	ArduinoJson::serializeJsonPretty(json_, json);
	ESP_LOGI(TAG, "%s", json.c_str());
}

/*
 *	Private Function Implementations
 */
long Config::getFileSize()
{
	fseek(file_, 0, SEEK_END);
	const long fileSize = ftell(file_);
	fseek(file_, 0, SEEK_SET);

	return fileSize;
}

bool Config::isFileEmpty()
{
	if (file_ == nullptr) {
		return true;
	}

	if (getFileSize() <= 0) {
		ESP_LOGW(TAG, "Configuration file %s is empty!", path_.c_str());
		return true;
	}

	return false;
}

std::vector<char> Config::readFileContent()
{
	const auto fileSize = getFileSize();
	std::vector<char> fileContent(fileSize);

	// Read data
	fread(fileContent.data(), 1, fileSize, file_);

	return fileContent;
}

bool Config::extractJsonFromFile(std::vector<char>& fileContent)
{
	if (ArduinoJson::deserializeJson(json_, fileContent.data(), fileContent.size()) !=
		ArduinoJson::DeserializationError::Ok) {
		ESP_LOGW(TAG, "Failed to parse config file %s", path_.c_str());
		return false;
	}

	return true;
}

void Config::startSaveTask()
{
	if (xTaskCreate(saveConfigTask, "SaveConfigTask", 4096, this, 2, &saveTask_) == pdPASS) {
		// Success
		return;
	}

	// Fail
	saveTask_ = nullptr;
	ESP_LOGW(TAG, "Failed to start task which saves the Config file regularily");
}

void Config::stopSaveTask()
{
	if (saveTask_ == nullptr) {
		return;
	}

	vTaskDelete(saveTask_);
	saveTask_ = nullptr;
}
