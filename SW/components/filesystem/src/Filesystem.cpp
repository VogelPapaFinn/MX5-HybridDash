#include "../Filesystem.hpp"

// C and C++ includes
#include <sys/stat.h>
#include <unistd.h>

// espidf includes
#include <bits/chrono.h>

#include "driver/gpio.h"

/*
 *	constexpr
 */
constexpr auto TAG = "Filesystem";

constexpr std::string SDCARD_BASE_PATH = "/sdcard";

constexpr std::string DATA_BASE_PATH = "/data";
constexpr std::string DATA_PARTITION_LABEL = "data";

constexpr std::string CONFIG_BASE_PATH = "/config";
constexpr std::string CONFIG_PARTITION_LABEL = "config";

constexpr gpio_num_t GPIO_CLK = GPIO_NUM_16;
constexpr gpio_num_t GPIO_CMD = GPIO_NUM_17;
constexpr gpio_num_t GPIO_D0 = GPIO_NUM_5;
constexpr gpio_num_t GPIO_D1 = GPIO_NUM_4;
constexpr gpio_num_t GPIO_D2 = GPIO_NUM_8;
constexpr gpio_num_t GPIO_D3 = GPIO_NUM_18;
constexpr uint8_t MAX_OPEN_FILES = 5;

/*
 *	Public Function Implementations
 */
Filesystem::Filesystem(bool mountSdCard, bool mountConfigPart, bool mountDataPart)
{
	bool warningOccured = false;
	bool errorOccured = false;

	/*
	 *	SD Card
	 */
	if (mountSdCard) {
		sdCardHost_.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // 40 MHz
		sdCardHost_.slot = SDMMC_HOST_SLOT_0;

		sdCardSlotConfig_.width = 4;
		sdCardSlotConfig_.clk = GPIO_CLK;
		sdCardSlotConfig_.cmd = GPIO_CMD;
		sdCardSlotConfig_.d0 = GPIO_D0;
		sdCardSlotConfig_.d1 = GPIO_D1;
		sdCardSlotConfig_.d2 = GPIO_D2;
		sdCardSlotConfig_.d3 = GPIO_D3;

		sdCardMountConfig_.format_if_mount_failed = false;
		sdCardMountConfig_.max_files = MAX_OPEN_FILES;

		sdCardMounted_ = esp_vfs_fat_sdmmc_mount(SDCARD_BASE_PATH.c_str(), &sdCardHost_, &sdCardSlotConfig_,
												 &sdCardMountConfig_, &sdCardHandler_) == ESP_OK;
		if (sdCardMounted_) {
			ESP_LOGI(TAG, "Mounted SD Card");
		}
		else {
			ESP_LOGW(TAG, "Couldn't mount SD Card");
			warningOccured = true;
		}
	}

	/*
	 *	Data Partition
	 */
	if (mountDataPart) {
		dataPartConfig_.base_path = DATA_BASE_PATH.c_str();
		dataPartConfig_.partition_label = DATA_PARTITION_LABEL.c_str();
		dataPartConfig_.max_files = MAX_OPEN_FILES;
		dataPartConfig_.format_if_mount_failed = false;

		dataPartMounted_ = esp_vfs_spiffs_register(&dataPartConfig_) == ESP_OK;
		if (dataPartMounted_) {
			ESP_LOGI(TAG, "Mounted data partition");
		}
		else {
			ESP_LOGE(TAG, "Couldn't mount data partition");
			errorOccured = true;
		}
	}

	/*
	 *	Config Partition
	 */
	if (mountConfigPart) {
		configPartConfig_.base_path = CONFIG_BASE_PATH.c_str();
		configPartConfig_.partition_label = CONFIG_PARTITION_LABEL.c_str();
		configPartConfig_.max_files = MAX_OPEN_FILES;
		configPartConfig_.format_if_mount_failed = true;

		configPartMounted_ = esp_vfs_spiffs_register(&configPartConfig_) == ESP_OK;
		if (configPartMounted_) {
			ESP_LOGI(TAG, "Mounted config partition");
		}
		else {
			ESP_LOGE(TAG, "Couldn't mount config partition");
			errorOccured = true;
		}
	}

	if (warningOccured) {
		ESP_LOGW(TAG, "Initialized with warnings");
	}
	else if (errorOccured) {
		ESP_LOGE(TAG, "Initialized with errors");
	}
	else {
		ESP_LOGI(TAG, "Initialized");
	}
}

bool Filesystem::doesFileExist(const std::string& path, Location location) const
{
	if (!isLocationMounted(location)) {
		return false;
	}

	struct stat buffer;
	return stat(buildFullPath(path, location).c_str(), &buffer) == 0;
}

bool Filesystem::createFile(const std::string& path, const Location location)
{
	if (!isLocationMounted(location)) {
		ESP_LOGI(TAG, "Couldn't create file: Location %d not mounted", location);
		return false;
	}

	if (doesFileExist(path, location)) {
		return false;
	}

	// For whatever reason fopen crashes with test.txt. I have absolutely no clue why but it costed me quite a few
	// hours until I found the crashes are caused by this :C
	if (path == "test.txt") {
		return false;
	}

	const std::string fullPath = buildFullPath(path, location);

	FILE* file = fopen(fullPath.c_str(), "a+");
	if (file == nullptr) {
		ESP_LOGW(TAG, "Couldn't create file %s", fullPath.c_str());
		return false;
	}

	fclose(file);

	return true;
}

FILE* Filesystem::openFile(const std::string& path, const std::string& mode, Location location)
{
	if (!isLocationMounted(location)) {
		ESP_LOGI(TAG, "Couldn't open file: Location %d not mounted", location);
		return nullptr;
	}

	if (!doesFileExist(path, location)) {
		return nullptr;
	}

	const std::string fullPath = buildFullPath(path, location);

	FILE* file = fopen(fullPath.c_str(), mode.c_str());
	if (file == nullptr) {
		ESP_LOGW(TAG, "Couldn't open file %s", fullPath.c_str());
		return nullptr;
	}

	return file;
}

bool Filesystem::deleteFile(const std::string& path, const Location location)
{
	if (!isLocationMounted(location)) {
		ESP_LOGI(TAG, "Couldn't open file: Location %d not mounted", location);
		return false;
	}

	if (!doesFileExist(path, location)) {
		return false;
	}

	const std::string fullPath = buildFullPath(path, location);

	if (remove(fullPath.c_str()) != 0) {
		ESP_LOGW(TAG, "Couldn't delete file %s", fullPath.c_str());
		return false;
	}

	return true;
}

bool Filesystem::doesDirectoryExist(const std::string& path, Location location) const
{
	if (location != SD_CARD) {
		ESP_LOGW(TAG, "The spiffs file system does not support directories!");
		return false;
	}

	if (!isLocationMounted(SD_CARD)) {
		return false;
	}

	const std::string fullPath = buildFullPath(path, location);

	struct stat stats;
	stat(fullPath.c_str(), &stats);
	return S_ISDIR(stats.st_mode);
}

bool Filesystem::createDirectory(const std::string& path, Location location) const
{
	if (location != SD_CARD) {
		ESP_LOGW(TAG, "The spiffs file system does not support directories!");
		return false;
	}

	if (!isLocationMounted(SD_CARD)) {
		return false;
	}

	if (doesDirectoryExist(path, SD_CARD)) {
		return false;
	}

	const std::string fullPath = buildFullPath(path, location);

	if (mkdir(fullPath.c_str(), S_IRWXU) != 0) {
		ESP_LOGW(TAG, "Failed to create directory %s", fullPath.c_str());
		return false;
	}

	return true;
}

bool Filesystem::deleteDirectory(const std::string& path, Location location) const
{
	if (location != SD_CARD) {
		ESP_LOGW(TAG, "The spiffs file system does not support directories!");
		return false;
	}

	if (!isLocationMounted(SD_CARD)) {
		return false;
	}

	if (!doesDirectoryExist(path, SD_CARD)) {
		return false;
	}

	const std::string fullPath = buildFullPath(path, location);

	if (rmdir(fullPath.c_str()) != 0) {
		ESP_LOGW(TAG, "Couldn't delete directory %s", fullPath.c_str());
		return false;
	}

	return true;
}

void Filesystem::test()
{
	bool success = true;

	// Slots for two files
	FILE* file1 = NULL;
	FILE* file2 = NULL;
	FILE* file3 = NULL;

	// Create two files, one on each internal partition
	if (createFile("hello.txt", CONFIG_PARTITION)) {
		file1 = openFile("hello.txt", "a+", CONFIG_PARTITION);
	}
	else {
		success = false;
	}
	if (createFile("hello.txt", DATA_PARTITION)) {
		file2 = openFile("hello.txt", "a+", DATA_PARTITION);
	}
	else {
		success = false;
	}
	// And one on the SD Card
	if (createFile("hello.txt", SD_CARD)) {
		file3 = openFile("hello.txt", "a+", SD_CARD);
	}
	else {
		success = false;
	}

	/* TESTING */

	// Close the files
	if (file1 != NULL) {
		fclose(file1);
	}
	else {
		success = false;
	}
	if (file2 != NULL) {
		fclose(file2);
	}
	else {
		success = false;
	}
	if (file3 != NULL) {
		fclose(file3);
	}
	else {
		success = false;
	}

	// Delete the files
	if (!deleteFile("hello.txt", CONFIG_PARTITION)) {
		success = false;
	}
	if (!deleteFile("hello.txt", DATA_PARTITION)) {
		success = false;
	}
	if (!deleteFile("hello.txt", SD_CARD)) {
		success = false;
	}

	// Create a directory "location" on the SD Card
	if (!createDirectory("location", SD_CARD)) {
		success = false;
	}

	// Check if these locations exists on the SD Card
	if (!doesDirectoryExist("location", SD_CARD)) {
		success = false;
	}

	// Reset the file pointers
	file1 = NULL;
	file2 = NULL;

	// Create new file in the newly created directory
	if (createFile("location/hello.txt", SD_CARD)) {
		file1 = openFile("hello.txt", "a+", SD_CARD);
	}
	else {
		success = false;
	}

	// Close the file again
	if (file1 != NULL) {
		fclose(file1);
	}

	// Then delete it
	if (!deleteFile("location/hello.txt", SD_CARD)) {
		success = false;
	}

	// Then delete the directory
	if (!deleteDirectory("location", SD_CARD)) {
		success = false;
	}

	if (success) {
		ESP_LOGI(TAG, "All Filesystem tests succeeded");
	}
	else {
		ESP_LOGW(TAG, "At least one filesystem test failed");
	}
}

/*
 *	Private Function Implementations
 */
bool Filesystem::isLocationMounted(const Location& location) const
{
	switch (location) {
		case SD_CARD:
			return sdCardMounted_;
		case DATA_PARTITION:
			return dataPartMounted_;
		case CONFIG_PARTITION:
			return configPartMounted_;
		default:
			return false;
	}
}

std::string Filesystem::buildFullPath(const std::string& path, const Location& location)
{
	switch (location) {
		case SD_CARD:
			return SDCARD_BASE_PATH + "/" + path;
		case DATA_PARTITION:
			return DATA_BASE_PATH + "/" + path;
		case CONFIG_PARTITION:
			return CONFIG_BASE_PATH + "/" + path;
		default:
			return path;
	}
}
