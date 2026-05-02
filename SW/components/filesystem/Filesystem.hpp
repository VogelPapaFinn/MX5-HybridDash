#pragma once

// C++ includes
#include <string>

// espidf includes
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

class Filesystem
{
public:
	/*
	 *	Public Enums
	 */
	enum Location
	{
		SD_CARD,
		DATA_PARTITION,
		CONFIG_PARTITION,
	};

	/*
	 *	Public Functions
	 */
	static Filesystem* get(bool mountSdCard = true, bool mountSpiffs = true);

	bool doesFileExist(const std::string& path, Location location);

	bool createFile(const std::string& path, Location location);

	FILE* openFile(const std::string& path, const std::string& mode, Location location);

	bool deleteFile(const std::string& path, const Location location);

	bool doesDirectoryExist(const std::string& path, Location location) const;

	bool createDirectory(const std::string& path, Location location) const;

	bool deleteDirectory(const std::string& path, Location location) const;

	void test();
private:
	/*
	 *	Private Functions
	 */
	Filesystem(bool mountSdCard = true, bool mountSpiffs = true);

	bool isLocationMounted(const Location& location) const;

	static std::string buildFullPath(const std::string& path, const Location& location);

	/*
	 *	Private Variables
	 */
	static Filesystem* instance_;

	/* SD Card Stuff */
	bool sdCardInserted_ = false;
	bool sdCardMounted_ = false;

	sdmmc_card_t* sdCardHandler_ = nullptr;
	sdmmc_host_t sdCardHost_ = SDMMC_HOST_DEFAULT();
	sdmmc_slot_config_t sdCardSlotConfig_ = SDMMC_SLOT_CONFIG_DEFAULT();
	esp_vfs_fat_sdmmc_mount_config_t sdCardMountConfig_;

	/* Data Partition Stuff */
	bool dataPartMounted_ = false;
	esp_vfs_spiffs_conf_t dataPartConfig_;

	/* Config Partition Stuff */
	bool configPartMounted_ = false;
	esp_vfs_spiffs_conf_t configPartConfig_;
};
