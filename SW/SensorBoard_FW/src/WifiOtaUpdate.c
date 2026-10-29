#include "WifiOtaUpdate.h"

// Project includes
#include "Drivers/Wifi.h"

// espidf includes
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_log.h>
#include <esp_crt_bundle.h>

/*
 *	Private defines
 */
#define CONNECTION_TIMEOUT_MS 15000

/*
 *	Public function implementations
 */
bool wifiOtaUpdateExecute()
{
	// Are we connected to WiFi?
	if (!wifiIsConnected()) {
		ESP_LOGW("WifiOtaUpdate", "Can't update: Not connected to Wifi!");

		return false;
	}

	// Are we not hosting an AP?
	if (wifiGetType() == HOST_AP) {
		ESP_LOGW("WifiOtaUpdate", "Can't update: Hosting an AP without internet connection!");

		return false;
	}

	// Logging
	ESP_LOGI("WifiOtaUpdate", "Starting OTA Update...");

	// Create the http config
	esp_http_client_config_t httpConfig = {
		.url = "https://github.com/VogelPapaFinn/MX5-HybridDash/releases/download/TEST/SensorBoard.bin",
		.crt_bundle_attach = esp_crt_bundle_attach,
		.timeout_ms = CONNECTION_TIMEOUT_MS,
		.disable_auto_redirect = false,
		.keep_alive_enable = true,
		.buffer_size = 4096,
		.buffer_size_tx = 2048,
	};

	// Create the OTA config
	const esp_https_ota_config_t otaConfig = {
		.http_config = &httpConfig
	};

	// Download Update File
	ESP_LOGI("WifiOtaUpdate", "Downloading OTA Update...");
	const esp_err_t result = esp_https_ota(&otaConfig);

	// Success
	if (result == ESP_OK) {
		ESP_LOGI("WifiOtaUpdate", "OTA Update successful!");
		return true;
	}

	// Fail
	ESP_LOGE("WifiOtaUpdate", "OTA Update failed!");
	return false;
}
