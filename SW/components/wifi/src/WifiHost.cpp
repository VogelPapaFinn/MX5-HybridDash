#include "../WifiHost.hpp"

// espidf includes
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

/*
 *	constexpr
 */
constexpr auto TAG = "WifiHost";

constexpr auto JSON_HOST_WIFI = "WifiHost";
constexpr auto JSON_SSID = "ssid";
constexpr auto JSON_PASSWORD = "password";

/*
 *	Public Function Implementations
 */
WifiHost::WifiHost() :
	Wifi(WIFI_TYPE::HOST),
	wifiConfig_()
{
}

WifiHost::~WifiHost()
{
	WifiHost::stop();
}

bool WifiHost::start()
{
	if (connected_) {
		return false;
	}

	/*
	 *	Initialization
	 */
	ESP_LOGI(TAG, "Initializing Wifi driver");

	if (ssid_.empty() || password_.empty()) {
		ESP_LOGE(TAG, "Failed to start AP. SSID or Password are empty");
		return false;
	}

	if (nvs_flash_init() != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initiate NVS Flash");
		return false;
	}

	if (esp_netif_init() != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initiate ESP NETIF");
		return false;
	}

	if (esp_event_loop_create_default() != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initiate ESP event loop");
		return false;
	}

	/*
	 *	Starting AP
	 */
	ESP_LOGI(TAG, "Starting AP");

	espNetif_ = esp_netif_create_default_wifi_ap();
	if (espNetif_ == nullptr) {
		ESP_LOGE(TAG, "Couldn't create ESP NETIF Default AP");
		return false;
	}

	if (esp_wifi_init(&wifiInitConfig_) != ESP_OK) {
		ESP_LOGE(TAG, "Couldn't initialize Wifi");
		return false;
	}

	wifiConfig_.ap.ssid_len = static_cast<uint8_t>(ssid_.size());
	wifiConfig_.ap.channel = 1;
	wifiConfig_.ap.authmode = WIFI_AUTH_WPA2_PSK;
	wifiConfig_.ap.max_connection = 4;
	strlcpy((char*)wifiConfig_.ap.ssid, ssid_.c_str(), sizeof(wifiConfig_.ap.ssid));
	strlcpy((char*)wifiConfig_.ap.password, password_.c_str(), sizeof(wifiConfig_.ap.password));

	if (esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK) {
		ESP_LOGE(TAG, "Couldn't set Wifi AP mode.");
		return false;
	}

	if (esp_wifi_set_config(WIFI_IF_AP, &wifiConfig_) != ESP_OK) {
		ESP_LOGE(TAG, "Couldn't set Wifi AP config.");
		return false;
	}

	if (esp_wifi_start() != ESP_OK) {
		ESP_LOGE(TAG, "Couldn't start Wifi AP.");
		return false;
	}

	// Everything worked
	connected_ = true;
	if (callOnSuccess_) {
		callOnSuccess_();
	}

	return true;
}

void WifiHost::stop()
{
	if (!connected_) {
		return;
	}

	esp_wifi_stop();

	esp_netif_destroy_default_wifi(espNetif_);
	espNetif_ = nullptr;

	esp_wifi_deinit();

	esp_event_loop_delete_default();
	esp_netif_deinit();
	nvs_flash_deinit();
}
