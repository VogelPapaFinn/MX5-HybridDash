#include "SdCard.hpp"

/*
 *   constexpr
 */

/*
 *   Public Function Implementations
 */
SdCard::SdCard(gpio_num_t gpioClk, gpio_num_t gpioCmd, gpio_num_t gpioD0, gpio_num_t gpioD1, gpio_num_t gpioD2,
			   gpio_num_t gpioD3)
{
	gpioClk_ = gpioClk;
	gpioCmd_ = gpioCmd;
	gpioD0_ = gpioD0;
	gpioD1_ = gpioD1;
	gpioD2_ = gpioD2;
	gpioD3_ = gpioD3;

	host_.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // 40 MHz
	host_.slot = SDMMC_HOST_SLOT_0;

	slotConfig_.width = 4;
	slotConfig_.clk = gpioClk_;
	slotConfig_.cmd = gpioCmd_;
	slotConfig_.d0 = gpioD0_;
	slotConfig_.d1 = gpioD1_;
	slotConfig_.d2 = gpioD2_;
	slotConfig_.d3 = gpioD3_;

	// sdCardMounted_ = esp_vfs_fat_sdmmc_mount(SDCARD_BASE_PATH.c_str(), &sdCardHost_, &sdCardSlotConfig_,
	// 										 &sdCardMountConfig_, &sdCardHandler_) == ESP_OK;
	// if (sdCardMounted_) {
	// 	ESP_LOGI(TAG, "Mounted SD Card");
	// }
	// else {
	// 	ESP_LOGW(TAG, "Couldn't mount SD Card");
	// 	warningOccured = true;
	// }
}
