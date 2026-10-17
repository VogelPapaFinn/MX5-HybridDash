#include "State/Startup.hpp"

// Project includes
#include "../config/DefaultConfig.hpp"

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
	startEspEventLoop();

	// Activate CAN
	can_->initialize();
	can_->enable();

	// Setup ADC
	setupAdc1();

	// Load config
	if (!loadConfig()) {
		loadDefaultConfig();
	}
}

/*
 *   Private Function Implementations
 */
void Startup::startEspEventLoop()
{
	if (esp_event_loop_create_default() == ESP_OK) {
		return;
	}

	ESP_LOGE(TAG, "Error creating default event loop. Rebooting...");
	esp_restart();
}

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
