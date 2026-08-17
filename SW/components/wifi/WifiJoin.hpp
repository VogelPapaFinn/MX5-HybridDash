#pragma once

// Project includes
#include "Wifi.hpp"

// C++ includes
#include <functional>

// Project includes
#include "Config.hpp"

// espidf includes
#include "esp_wifi.h"

class WifiJoin : public Wifi
{
public:
	WifiJoin(SystemContext* p_sysCon);

	~WifiJoin() override;

	bool start() override;

	void stop() override;

	void setMasterIp(std::array<uint8_t, 4> ip);

	std::array<uint8_t, 4>& getMasterIp();

	/*
	 *	Private ISR
	 */
	void ipEventHandler(esp_event_base_t p_eventBase, int32_t eventId, void* p_eventData);
private:
	/*
	 *	Variables
	 */
	bool active_ = false;
	
	esp_netif_t* espNetif_ = nullptr;
	wifi_init_config_t wifiInitConfig_ = WIFI_INIT_CONFIG_DEFAULT();
	wifi_config_t wifiConfig_;

	esp_event_handler_instance_t wifiEventHandlerInstance_ = nullptr;
	esp_event_handler_instance_t ipEventHandlerInstance_ = nullptr;

	std::array<uint8_t, 4> masterIp_;
};
