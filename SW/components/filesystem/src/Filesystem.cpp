#include "../Filesystem.hpp"

// C and C++ includes
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <unistd.h>

// espidf includes
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "Filesystem";

/*
 *	Public Function Implementations
 */
Filesystem::Filesystem(std::vector<Location> locations)
{
	locations_ = std::move(locations);

	if (mountAllLocations()) {
		ESP_LOGI(TAG, "Initialized");
	}
	else {
		ESP_LOGW(TAG, "Initialization was completed with some warnings/errors. See log for more information");
	}
}

bool Filesystem::mountAllLocations()
{
	if (locations_.empty()) {
		ESP_LOGW(TAG, "Couldn't mount locations. Vector was empty");
		return false;
	}

	bool result = true;

	for (auto& l : locations_) {
		if (locationInformation_.contains(l.id)) {
			ESP_LOGW(TAG, "There is already a location registered with ID %d. Skipping", l.id);
			continue;
		}

		// FatFS
		if (l.fsType == FAT_FS) {
			// Skip if necessary
			if (l.sdCard == nullptr) {
				ESP_LOGE(TAG, "FAT location %d has no SdCard", l.id);
				result = false;

				continue;
			}

			// Mount
			FilesystemInstance fsI{.mounted = l.sdCard->mount(l.basePath, l.maxOpenFiles, l.formatIfMountFails),
								   .basePath = l.basePath};

			if (fsI.mounted) {
				ESP_LOGI(TAG, "Mounted SD Card at %s", l.basePath.c_str());
			}
			else {
				ESP_LOGW(TAG, "Couldn't mount SD Card at %s", l.basePath.c_str());
			}

			locationInformation_[l.id] = fsI;

			continue;
		}

		// LittleFS
		if (l.fsType == LITTLE_FS) {
			const esp_vfs_littlefs_conf_t config = {.base_path = l.basePath.c_str(),
													.partition_label = l.partitionLabel.c_str(),
													.format_if_mount_failed = l.formatIfMountFails};

			// Mount FS
			FilesystemInstance fsI{.mounted = false, .basePath = l.basePath};
			fsI.mounted = esp_vfs_littlefs_register(&config) == ESP_OK;

			// Logging
			if (fsI.mounted) {
				ESP_LOGI(TAG, "Mounted LittleFS partition with base path %s", fsI.basePath.c_str());
			}
			else {
				result = false;
				ESP_LOGE(TAG, "Couldn't mount LittleFS partition with base path %s", fsI.basePath.c_str());
			}

			// Keep track of its information
			locationInformation_[l.id] = fsI;

			continue;
		}
	}

	return result;
}

bool Filesystem::doesFileExist(const std::string& path, uint8_t locationId) const
{
	if (!isLocationMounted(locationId)) {
		return false;
	}

	struct stat buffer;
	return stat(buildFullPath(path, locationId).c_str(), &buffer) == 0;
}

bool Filesystem::createFile(const std::string& path, uint8_t locationId)
{
	if (!isLocationMounted(locationId)) {
		ESP_LOGI(TAG, "Couldn't create file: Location %d not mounted", locationId);
		return false;
	}

	if (doesFileExist(path, locationId)) {
		return false;
	}

	// For whatever reason fopen crashes with test.txt. I have absolutely no clue why but it costed me quite a few
	// hours until I found the crashes are caused by this :C
	if (path == "test.txt") {
		return false;
	}

	const std::string fullPath = buildFullPath(path, locationId);

	FILE* file = fopen(fullPath.c_str(), "a+");
	if (file == nullptr) {
		ESP_LOGW(TAG, "Couldn't create file %s", fullPath.c_str());
		return false;
	}

	fclose(file);

	return true;
}

FILE* Filesystem::openFile(const std::string& path, const std::string& mode, uint8_t locationId)
{
	if (!isLocationMounted(locationId)) {
		ESP_LOGI(TAG, "Couldn't open file: Location %d not mounted", locationId);
		return nullptr;
	}

	if (!doesFileExist(path, locationId)) {
		return nullptr;
	}

	const std::string fullPath = buildFullPath(path, locationId);

	FILE* file = fopen(fullPath.c_str(), mode.c_str());
	if (file == nullptr) {
		ESP_LOGW(TAG, "Couldn't open file %s", fullPath.c_str());
		return nullptr;
	}

	return file;
}

bool Filesystem::deleteFile(const std::string& path, uint8_t locationId)
{
	if (!isLocationMounted(locationId)) {
		ESP_LOGI(TAG, "Couldn't open file: Location %d not mounted", locationId);
		return false;
	}

	if (!doesFileExist(path, locationId)) {
		return false;
	}

	const std::string fullPath = buildFullPath(path, locationId);

	if (remove(fullPath.c_str()) != 0) {
		ESP_LOGW(TAG, "Couldn't delete file %s", fullPath.c_str());
		return false;
	}

	return true;
}

bool Filesystem::doesDirectoryExist(const std::string& path, uint8_t locationId) const
{
	if (!isLocationMounted(locationId)) {
		return false;
	}

	const std::string fullPath = buildFullPath(path, locationId);

	struct stat stats;
	return stat(fullPath.c_str(), &stats) == 0 && S_ISDIR(stats.st_mode);
}

bool Filesystem::createDirectory(const std::string& path, uint8_t locationId) const
{
	if (!isLocationMounted(locationId)) {
		return false;
	}

	if (doesDirectoryExist(path, locationId)) {
		return false;
	}

	const std::string fullPath = buildFullPath(path, locationId);

	if (mkdir(fullPath.c_str(), S_IRWXU) != 0) {
		ESP_LOGW(TAG, "Failed to create directory %s", fullPath.c_str());
		return false;
	}

	return true;
}

bool Filesystem::deleteDirectory(const std::string& path, uint8_t locationId) const
{
	if (!isLocationMounted(locationId)) {
		return false;
	}

	if (!doesDirectoryExist(path, locationId)) {
		return false;
	}

	const std::string fullPath = buildFullPath(path, locationId);

	if (rmdir(fullPath.c_str()) != 0) {
		ESP_LOGW(TAG, "Couldn't delete directory %s", fullPath.c_str());
		return false;
	}

	return true;
}

std::vector<std::string> Filesystem::getAllFileNamesInDirectory(const std::string& pathToDir,
																const std::string& pattern, uint8_t locationId) const
{
	if (!isLocationMounted(locationId)) {
		return {};
	}

	const std::string fullPath = buildFullPath(pathToDir, locationId);

	DIR* dir = opendir(fullPath.c_str());
	if (!dir) {
		ESP_LOGW(TAG, "Couldn't open directory %s on location %d", fullPath.c_str(), locationId);
		return {};
	}

	std::vector<std::string> output;

	struct dirent* entry = nullptr;
	do {
		entry = readdir(dir);

		if (entry == nullptr) {
			break;
		}

		// Ignore everything besides files
		if (entry->d_type != DT_REG) {
			continue;
		}

		if (fnmatch(pattern.c_str(), entry->d_name, 0) == 0) {
			output.push_back(std::string(entry->d_name));
		}
	}
	while (entry != nullptr);

	closedir(dir);

	return output;
}

/*
 *	Private Function Implementations
 */
bool Filesystem::isLocationMounted(uint8_t locationId) const
{
	if (!locationInformation_.contains(locationId)) {
		return false;
	}

	return locationInformation_.at(locationId).mounted;
}

std::string Filesystem::buildFullPath(const std::string& path, uint8_t locationId) const
{
	std::string fullPath = locationInformation_.at(locationId).basePath;
	fullPath += "/";
	fullPath += path;

	return fullPath;
}
