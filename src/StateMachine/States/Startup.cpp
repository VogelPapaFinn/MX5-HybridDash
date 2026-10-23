#include "StateMachine/States/Startup.hpp"

// Project includes
#include "../config/DefaultConfig.hpp"
#include "StateMachine/StateEvents.hpp"

/*
 *   constexpr
 */
constexpr auto TAG = "Startup";
constexpr auto CONFIG_NAME = "config.json";

/*
 *   Public Function Implementations
 */
Startup::Startup(Filesystem* p_fs, Config** p_config, Can* p_can, GuiHandler* p_guiHandler)
{
	fs_ = p_fs;
	configPtr_ = p_config;
	can_ = p_can;
	guiHandler_ = p_guiHandler;
}

void Startup::enter()
{
	// Activate CAN
	can_->initialize();
	can_->enable();

	// Load config
	if (!loadConfig()) {
		loadDefaultConfig();
	}

	// Enter the WaitingForConfiguration state?
	if (!isConfigValid()) {
		esp_event_post(STATE_EVENT_BASE, STATE_EVENT_ID::CONFIG_INVALID, nullptr, 0, portMAX_DELAY);
		return;
	}

	loadScreen();

	forceCanId();

	// Enter the Operation state
	esp_event_post(STATE_EVENT_BASE, STATE_EVENT_ID::CONFIG_VALID, nullptr, 0, portMAX_DELAY);
}

void Startup::quit() {}

/*
 *   Private Function Implementations
 */
bool Startup::loadConfig()
{
	if (*configPtr_ == nullptr) {
		*configPtr_ = new Config(fs_, CONFIG_NAME);
	}

	return (*configPtr_)->open();
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

void Startup::loadScreen()
{
	auto json = (*configPtr_)->getReadAccess();

	if (json["screen"] && json["screen"].as<int>() >= 0) {
		guiHandler_->loadScreen(json["screen"].as<GuiHandler::SCREEN>());
	}
}

bool Startup::isConfigValid()
{
	const auto json = (*configPtr_)->getReadAccess();
	if (!json["canID"] || !json["screen"] || !json["rotation"]) {
		return false;
	}

	if (json["canID"].as<unsigned int>() == 0) {
		return false;
	}

	const auto& screen = json["screen"].as<unsigned int>();
	if (screen < 0 || screen > 2) {
		return false;
	}

	const auto& rotation = json["rotation"].as<unsigned int>();
	if (rotation != 0 && rotation != 1) {
		return false;
	}

	return true;
}

void Startup::forceCanId()
{
	const auto json = (*configPtr_)->getReadAccess();
	if (!json["canID"]) {
		return;
	}

	// Force Can ID
	can_->forceSenderId(json["canID"].as<int>());
}
