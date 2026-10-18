#include "State/Startup.hpp"

// Project includes
#include "../config/DefaultConfig.hpp"
#include "State/StateEvents.hpp"

// espidf includes
#include "esp_event.h"
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr auto TAG = "Startup";

constexpr adc_oneshot_unit_init_cfg_t ADC1_UNIT_CONFIG = {.unit_id = ADC_UNIT_1, .ulp_mode = ADC_ULP_MODE_DISABLE};

constexpr auto CONFIG_NAME = "config.json";

/*
 *   Public Function Implementations
 */
Startup::Startup(Filesystem* p_fs, Config* p_config, Can* p_can, adc_oneshot_unit_handle_t p_adc1Handle)
{
	fs_ = p_fs;
	config_ = p_config;
	can_ = p_can;
	adc1Handle_ = p_adc1Handle;
}

void Startup::enter()
{
	// Activate CAN
	can_->initialize();
	can_->enable();

	// Setup ADC
	setupAdc1();

	// Load config
	if (!loadConfig()) {
		loadDefaultConfig();
	}

	// Enter the ConfigureDisplays state?
	if (isFirstStartup()) {
		esp_event_post(STATE_EVENT_BASE, STATE_EVENT_ID::FIRST_STARTUP, nullptr, 0, portMAX_DELAY);
		return;
	}

	// Enter the UpdateDisplays state?
	if (isUpdateAvailable()) {
		esp_event_post(STATE_EVENT_BASE, STATE_EVENT_ID::UPDATE_AVAILABLE, nullptr, 0, portMAX_DELAY);
		return;
	}

	// Enter the operation state
	esp_event_post(STATE_EVENT_BASE, STATE_EVENT_ID::FINISHED, nullptr, 0, portMAX_DELAY);
}

void Startup::quit() {}

/*
 *   Private Function Implementations
 */
void Startup::setupAdc1()
{
	if (adc_oneshot_new_unit(&ADC1_UNIT_CONFIG, &adc1Handle_) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize ADC1");
	}
}

bool Startup::loadConfig()
{
	if (config_ == nullptr) {
		config_ = new Config(fs_, CONFIG_NAME);
	}

	return config_->open();
}

bool Startup::loadDefaultConfig()
{
	// Create file if necessary
	if (!fs_->doesFileExist(CONFIG_NAME, Filesystem::CONFIG_PARTITION)) {
		fs_->createFile(CONFIG_NAME, Filesystem::CONFIG_PARTITION);
	}

	// Open file for writing
	FILE* file = fs_->openFile(CONFIG_NAME, "w", Filesystem::CONFIG_PARTITION);

	// Print default config to it
	if (fprintf(file, "%s", DEFAULT_CONFIG) <= 0) {
		ESP_LOGW(TAG, "Failed to create default config");
		return false;
	}

	fflush(file);
	fclose(file);

	return true;
}

bool Startup::isFirstStartup()
{
	const auto json = config_->getReadAccess();
	if (!json["FirstStartup"]) {
		return true;
	}

	if (json["FirstStartup"].as<bool>()) {
		return true;
	}

	return false;
}

bool Startup::isUpdateAvailable() { return fs_->doesFileExist("display_update.bin", Filesystem::DATA_PARTITION); }
