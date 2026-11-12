#pragma once

// Project includes
#include "SdCard.hpp"

// C++ includes
#include <string>
#include <unordered_map>
#include <vector>


// espidf includes
#include "esp_littlefs.h"


class Filesystem
{
public:
	/*
	 *	Public Enums/Structs
	 */
	enum FILESYSTEM_TYPE
	{
		LITTLE_FS,
		FAT_FS
	};

	struct Location
	{
		uint8_t id = 0;
		FILESYSTEM_TYPE fsType = LITTLE_FS;
		std::string basePath = "";
		std::string partitionLabel = "";
		bool formatIfMountFails = false;

		// Only FAT_FS
		SdCard* sdCard = nullptr;
		uint8_t maxOpenFiles = 5;
	};

	/*
	 *	Public Functions
	 */
	Filesystem(std::vector<Location> locations);
	Filesystem(const Filesystem&) = delete;

	bool doesFileExist(const std::string& path, uint8_t locationId) const;

	bool createFile(const std::string& path, uint8_t locationId);

	FILE* openFile(const std::string& path, const std::string& mode, uint8_t locationId);

	bool deleteFile(const std::string& path, uint8_t locationId);

	bool doesDirectoryExist(const std::string& path, uint8_t locationId) const;

	bool createDirectory(const std::string& path, uint8_t locationId) const;

	bool deleteDirectory(const std::string& path, uint8_t locationId) const;

	std::vector<std::string> getAllFileNamesInDirectory(const std::string& pathToDir, const std::string& pattern,
														uint8_t locationId) const;

private:
	/*
	 *	Private Functions
	 */
	bool mountAllLocations();

	bool isLocationMounted(uint8_t locationId) const;

	std::string buildFullPath(const std::string& path, uint8_t locationId) const;

	/*
	 *	Private Structs
	 */
	struct FilesystemInstance
	{
		bool mounted = false;
		std::string basePath = "";
	};

	std::vector<Location> locations_;
	std::unordered_map<uint8_t, FilesystemInstance> locationInformation_;
};
