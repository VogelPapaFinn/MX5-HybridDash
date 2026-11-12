#pragma once

// espidf includes
#include <driver/gpio.h>
#include "driver/sdmmc_host.h"

class SdCard
{
public:
	SdCard(gpio_num_t gpioClk, gpio_num_t gpioCmd, gpio_num_t gpioD0, gpio_num_t gpioD1, gpio_num_t gpioD2,
		   gpio_num_t gpioD3);

private:
	gpio_num_t gpioClk_;
	gpio_num_t gpioCmd_;
	gpio_num_t gpioD0_;
	gpio_num_t gpioD1_;
	gpio_num_t gpioD2_;
	gpio_num_t gpioD3_;

	sdmmc_card_t* handler_ = nullptr;
	sdmmc_host_t host_ = SDMMC_HOST_DEFAULT();
	sdmmc_slot_config_t slotConfig_ = SDMMC_SLOT_CONFIG_DEFAULT();
};
