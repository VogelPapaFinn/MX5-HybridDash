// Project includes
#include "Config.hpp"
#include "Filesystem.hpp"
#include "FilesystemLocations.hpp"
#include "Handler/GuiHandler.hpp"
#include "StateMachine/StateMachine.hpp"
#include "StateMachine/States/Configuration.hpp"
#include "StateMachine/States/Operation.hpp"
#include "StateMachine/States/Startup.hpp"
#include "StateMachine/States/Update.hpp"
#include "StateMachine/States/WaitingForConfiguration.hpp"

// espidf includes
#include "esp_event.h"
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "main";

constexpr gpio_num_t GPIO_CAN_RX = GPIO_NUM_39;
constexpr gpio_num_t GPIO_CAN_TX = GPIO_NUM_40;

constexpr std::string CONFIG_BASE_PATH = "/config";
constexpr std::string CONFIG_PARTITION_LABEL = "config";
constexpr std::string LOG_BASE_PATH = "/log";
constexpr std::string LOG_PARTITION_LABEL = "log";

/*
 *	Helper functions
 */
void startEspEventLoop()
{
	if (esp_event_loop_create_default() == ESP_OK) {
		return;
	}

	ESP_LOGE(TAG, "Error creating default event loop. Rebooting...");
	esp_restart();
}

/*
 *	main function
 */
extern "C" void app_main(void)
{
	// NEEDED FOR DEBUGGING
	vTaskDelay(pdMS_TO_TICKS(250));

	// Print Log Headers
	ESP_LOGI(TAG, "--- --- --- --- --- --- ---");
	ESP_LOGI(TAG, "Startup");
	ESP_LOGI(TAG, "UPDATE");

	startEspEventLoop();

	// Drivers
	Filesystem fs({
		{.id = FsLocation::CONFIG_PARTITION,
		 .basePath = CONFIG_BASE_PATH,
		 .partitionLabel = CONFIG_PARTITION_LABEL,
		 .formatIfMountFails = true},
		{.id = FsLocation::LOG_PARTITION,
		 .basePath = LOG_BASE_PATH,
		 .partitionLabel = LOG_PARTITION_LABEL,
		 .formatIfMountFails = true},
	});
	Config config(&fs, FsLocation::CONFIG_PARTITION);
	Can can(GPIO_CAN_RX, GPIO_CAN_TX);
	GuiHandler guiHandler;

	// States
	Startup startupState(&fs, &config, &can, &guiHandler);
	WaitingForConfiguration waitingState(&can);
	Operation operationState(&can, &guiHandler);
	Configuration configurationState(&fs, &config, &can);
	Update updateState(&can, &guiHandler);

	// StateMachine
	StateMachine stateMachine(
		&startupState,
		{
			// Source: Startup
			{.from = &startupState, .signal = STATE_EVENT_ID::CONFIG_INVALID, .to = &waitingState},
			{.from = &startupState, .signal = STATE_EVENT_ID::CONFIG_VALID, .to = &operationState},

			// Source: WaitingForConfiguration
			{.from = &waitingState, .signal = STATE_EVENT_ID::ENTER_CONFIGURATION_MODE, .to = &configurationState},

			// Source: Operation
			{.from = &operationState, .signal = STATE_EVENT_ID::ENTER_UPDATE_MODE, .to = &updateState},
			{.from = &operationState, .signal = STATE_EVENT_ID::ENTER_CONFIGURATION_MODE, .to = &configurationState},

			// Source: Update
			{.from = &updateState, .signal = STATE_EVENT_ID::UPDATE_FAILED, .to = &operationState},
		});
	stateMachine.start();

	vTaskSuspend(nullptr);
}
