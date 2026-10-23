// Project includes
#include "Config.hpp"
#include "Filesystem.hpp"
#include "Handler/GuiHandler.hpp"
#include "StateMachine/StateMachine.hpp"
#include "StateMachine/States/Configuration.hpp"
#include "StateMachine/States/Operation.hpp"
#include "StateMachine/States/Startup.hpp"
#include "StateMachine/States/Update.hpp"
#include "StateMachine/States/WaitingForConfiguration.hpp"

// espidf includes
#include "esp_event.h"

/*
 *	constexpr
 */
constexpr auto TAG = "main";

constexpr gpio_num_t GPIO_CAN_RX = GPIO_NUM_39;
constexpr gpio_num_t GPIO_CAN_TX = GPIO_NUM_40;

/*
 *	Private Static Variables
 */
static Filesystem g_fs;
static Config* g_config = nullptr;
static Can g_can(GPIO_CAN_RX, GPIO_CAN_TX);
static GuiHandler g_guiHandler;

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

	startEspEventLoop();

	// Handlers

	// States
	Startup startupState(&g_fs, &g_config, &g_can, &g_guiHandler);
	WaitingForConfiguration waitingState(&g_can);
	Operation operationState(&g_can, &g_guiHandler);
	Configuration configurationState(&g_fs, g_config, &g_can);
	Update updateState(&g_can);

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

			// Source: Update
			{.from = &updateState, .signal = STATE_EVENT_ID::UPDATE_FAILED, .to = &operationState},
		});
	stateMachine.start();

	while (true) {
		vTaskDelay(portMAX_DELAY);
	}
}


TODO:
	// - Both firmwares compile. FLASH and test! C:
