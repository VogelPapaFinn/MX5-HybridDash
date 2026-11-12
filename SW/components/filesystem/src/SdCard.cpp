#include "SdCard.hpp"

// espidf includes
#include "esp_vfs_fat.h"

/*
 *   constexpr
 */
constexpr auto TAG = "SDCard";

/*
 *   Public Function Implementations
 */
SdCard::SdCard(gpio_num_t gpioClk, gpio_num_t gpioCmd, gpio_num_t gpioD0, gpio_num_t gpioD1, gpio_num_t gpioD2,
			   gpio_num_t gpioD3)
{
	host_.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // 40 MHz
	host_.slot = SDMMC_HOST_SLOT_0;

	slotConfig_.width = 4;
	slotConfig_.clk = gpioClk;
	slotConfig_.cmd = gpioCmd;
	slotConfig_.d0 = gpioD0;
	slotConfig_.d1 = gpioD1;
	slotConfig_.d2 = gpioD2;
	slotConfig_.d3 = gpioD3;
}

bool SdCard::mount(const std::string& basePath, uint8_t maxOpenFiles, bool formatIfMountFails)
{
	if (card_ != nullptr) {
		ESP_LOGW(TAG, "SD Card already mounted! Aborting mount");
		return false;
	}

	esp_vfs_fat_sdmmc_mount_config_t mountConfig = {};
	mountConfig.format_if_mount_failed = formatIfMountFails;
	mountConfig.max_files = maxOpenFiles;

	return esp_vfs_fat_sdmmc_mount(basePath.c_str(), &host_, &slotConfig_, &mountConfig, &card_) == ESP_OK;
}
