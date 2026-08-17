#include "../WifiJoin.hpp"

// espidf includes
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"

/*
 *	constexpr
 */
constexpr auto TAG = "WifiJoin";

constexpr auto HOST_NAME = "MX5-HybridDash";

constexpr auto JSON_JOIN_WIFI = "WifiJoin";
constexpr auto JSON_SSID = "ssid";
constexpr auto JSON_PASSWORD = "password";

constexpr esp_sntp_config config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
constexpr auto SNTP_TIMEOUT_MS = 10000;

/*
 *	Private Static ISRs
 */
static void staticWifiEventHandler(void* p_arg, const esp_event_base_t p_eventBase, const int32_t eventId,
                                   void* p_eventData)
{
	if (p_eventBase != WIFI_EVENT) {
		return;
	}

	if (eventId == WIFI_EVENT_STA_START) {
		esp_wifi_connect();

		return;
	}

	if (eventId == WIFI_EVENT_STA_DISCONNECTED) {
		esp_wifi_connect();

		static uint8_t failCounter = 0;
		failCounter++;
		if (failCounter >= 10) {
			esp_wifi_disconnect();
			esp_wifi_stop();

			failCounter = 0;
			ESP_LOGI(TAG, "Fail count reached. Aborting connection attempts");
		}

		return;
	}
}

static void staticIpEventHandler(void* p_arg, const esp_event_base_t p_eventBase, const int32_t eventId,
                                 void* p_eventData)
{
	if (p_arg == nullptr) {
		return;
	}

	auto w = static_cast<WifiJoin*>(p_arg);
	w->ipEventHandler(p_eventBase, eventId, p_eventData);
}

static void staticSynchronizeTimeTask(void* p_param)
{
	ESP_LOGI(TAG, "Initializing SNTP...");

	esp_netif_sntp_init(&config);

	if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(SNTP_TIMEOUT_MS)) != ESP_OK) {
		ESP_LOGW(TAG, "Failed to synchronize system time. Wifi/HTTPS operations may fail because of this!");
	}
	else {
		time_t now;
		time(&now);

		// Set timezone to Europe/Berlin
		setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
		tzset();

		// Logging
		char timeBuffer[64];
		tm timeinfo;
		localtime_r(&now, &timeinfo);
		strftime(timeBuffer, sizeof(timeBuffer), "%c", &timeinfo);
		ESP_LOGI(TAG, "Synchronized system time: %s", timeBuffer);
	}

	vTaskDelete(NULL);
}

/*
 *	Public Function Implementations
 */
WifiJoin::WifiJoin(SystemContext* p_sysCon) :
	Wifi(p_sysCon, WIFI_TYPE::JOIN),
	wifiConfig_()
{
}

WifiJoin::~WifiJoin()
{
	WifiJoin::stop();
}

bool WifiJoin::start()
{
	if (active_) {
		return false;
	}

	/*
	 *	Initialization
	 */
	ESP_LOGI(TAG, "Initializing Wifi driver");

	if (ssid_.empty() || password_.empty()) {
		ESP_LOGE(TAG, "Failed to join AP. SSID or Password are empty");
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
	 *	Joining AP
	 */
	ESP_LOGI(TAG, "Joining AP using SSID: '%s' and Password: '%s'", ssid_.c_str(), password_.c_str());

	espNetif_ = esp_netif_create_default_wifi_sta();
	if (espNetif_ == nullptr) {
		ESP_LOGE(TAG, "Couldn't create ESP NETIF Default STA");
		return false;
	}

	if (esp_netif_set_hostname(espNetif_, HOST_NAME) != ESP_OK) {
		ESP_LOGI(TAG, "Failed to set host name");
	}

	if (esp_wifi_init(&wifiInitConfig_) != ESP_OK) {
		ESP_LOGE(TAG, "Couldn't initialize Wifi");
		return false;
	}

	wifiConfig_.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
	strlcpy((char*)wifiConfig_.ap.ssid, ssid_.c_str(), sizeof(wifiConfig_.ap.ssid));
	strlcpy((char*)wifiConfig_.ap.password, password_.c_str(), sizeof(wifiConfig_.ap.password));

	if (esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &staticWifiEventHandler, nullptr,
	                                        &wifiEventHandlerInstance_) !=
		ESP_OK) {
		ESP_LOGE(TAG, "Couldn't register wifi handler instance for wifi events");
		return false;
	}

	if (esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &staticIpEventHandler, this,
	                                        &ipEventHandlerInstance_) !=
		ESP_OK) {
		ESP_LOGE(TAG, "Couldn't register wifi handler instance for ip events.");
		return false;
	}

	if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
		ESP_LOGE(TAG, "Couldn't set wifi AP mode.");
		return false;
	}

	if (esp_wifi_set_config(WIFI_IF_STA, &wifiConfig_) != ESP_OK) {
		ESP_LOGE(TAG, "Couldn't set wifi AP config.");
		return false;
	}

	if (esp_wifi_start() != ESP_OK) {
		ESP_LOGE(TAG, "Couldn't start connecting to wifi AP.");
		return false;
	}

	active_ = true;
	return true;
}

void WifiJoin::stop()
{
	esp_wifi_stop();

	esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &ipEventHandlerInstance_);
	esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandlerInstance_);

	esp_wifi_deinit();

	esp_netif_destroy_default_wifi(espNetif_);
	esp_event_loop_delete_default();
	esp_netif_deinit();
	nvs_flash_deinit();
}

void WifiJoin::setMasterIp(const std::array<uint8_t, 4> ip)
{
	masterIp_ = ip;
}

std::array<uint8_t, 4>& WifiJoin::getMasterIp()
{
	return masterIp_;
}

/*
 *	Private ISR Implementations
 */
void WifiJoin::ipEventHandler(const esp_event_base_t p_eventBase, const int32_t eventId, void* p_eventData)
{

	if (p_eventBase != IP_EVENT) {
		return;
	}

	if (p_eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
		const ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(p_eventData);

		ip_[0] = esp_ip4_addr1_16(&event->ip_info.ip);
		ip_[1] = esp_ip4_addr2_16(&event->ip_info.ip);
		ip_[2] = esp_ip4_addr3_16(&event->ip_info.ip);
		ip_[3] = esp_ip4_addr4_16(&event->ip_info.ip);

		connected_ = true;

		ESP_LOGI(TAG, "Got IP assigned: %d.%d.%d.%d", IP2STR(&event->ip_info.ip));

		if (callOnSuccess_) {
			callOnSuccess_();
		}

		// Initialize the SNTP for time synchronization
		if (xTaskCreate(staticSynchronizeTimeTask, "synchronizeTimeTask", 2048 * 4, nullptr, 0,
		                nullptr) != pdPASS) {
			ESP_LOGE(TAG, "Couldn't start time synchronization task!");
		}

		return;
	}
}
