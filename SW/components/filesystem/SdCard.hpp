#pragma once

// C++ includes
#include <string>

// espidf includes
#include <driver/gpio.h>
#include "driver/sdmmc_host.h"

class SdCard
{
public:
	SdCard(gpio_num_t gpioClk, gpio_num_t gpioCmd, gpio_num_t gpioD0, gpio_num_t gpioD1, gpio_num_t gpioD2,
		   gpio_num_t gpioD3);

	SdCard(const SdCard&) = delete;

	bool mount(const std::string& basePath, uint8_t maxOpenFiles, bool formatIfMountFails);

private:
	sdmmc_card_t* card_ = nullptr;
	sdmmc_host_t host_ = SDMMC_HOST_DEFAULT();
	sdmmc_slot_config_t slotConfig_ = SDMMC_SLOT_CONFIG_DEFAULT();
};
