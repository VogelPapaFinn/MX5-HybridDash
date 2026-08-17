#pragma once

// Project includes
#include "Config.hpp"
#include "Wifi.hpp"

// espidf includes
#include "esp_wifi.h"

class WifiHost : public Wifi
{
public:
	WifiHost(SystemContext* p_sysCon);

	~WifiHost() override;

	bool start() override;

	void stop() override;

private:
	/*
	 *	Variables
	 */
	esp_netif_t* espNetif_ = nullptr;
	wifi_init_config_t wifiInitConfig_ = WIFI_INIT_CONFIG_DEFAULT();
	wifi_config_t wifiConfig_;
};
