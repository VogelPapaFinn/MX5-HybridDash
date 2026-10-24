// Project includes
#include "Can.hpp"
#include "Driver/Display.hpp"
#include "Driver/KLine.hpp"
#include "Filesystem.hpp"
#include "Sensors/Handlers/SensorHandler.hpp"
#include "StateMachine/StateEvents.hpp"
#include "StateMachine/StateMachine.hpp"
#include "StateMachine/States/ConfigureDisplays.hpp"
#include "StateMachine/States/Operation.hpp"
#include "StateMachine/States/Startup.hpp"
#include "StateMachine/States/UpdateDisplays.hpp"

// espidf includes
#include <esp_adc/adc_oneshot.h>
#include <freertos/FreeRTOS.h>

//! \brief Main application tag.
constexpr auto TAG = "main";

//! \brief GPIO pin number for CAN receiver.
constexpr gpio_num_t GPIO_CAN_RX = GPIO_NUM_41;

//! \brief GPIO pin number for CAN transmitter.
constexpr gpio_num_t GPIO_CAN_TX = GPIO_NUM_40;

constexpr gpio_num_t GPIO_DISPLAY1 = GPIO_NUM_13;
constexpr gpio_num_t GPIO_DISPLAY2 = GPIO_NUM_14;
constexpr gpio_num_t GPIO_DISPLAY3 = GPIO_NUM_21;

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
	vTaskDelay(pdMS_TO_TICKS(100));

	// Print Log Headers
	ESP_LOGI(TAG, "--- --- --- --- --- --- ---");
	ESP_LOGI(TAG, "Startup");

	startEspEventLoop();

	// Drivers
	Filesystem fs;
	KLine kline;
	adc_oneshot_unit_handle_t adc1 = nullptr;
	Config config;
	Can can(GPIO_CAN_RX, GPIO_CAN_TX);
	std::vector<Display> displays = {Display(GPIO_DISPLAY1, CAN_MASTER_ID + 1, 1, false),
									 Display(GPIO_DISPLAY2, CAN_MASTER_ID + 2, 0, true),
									 Display(GPIO_DISPLAY3, CAN_MASTER_ID + 3, 2, false)};

	// Handlers
	SensorsData sensorsData(&can);
	SensorHandler sensorHandler(adc1, &sensorsData);

	// States
	Startup startupState(&fs, &config, &can, adc1);
	Operation operationState(&sensorHandler);
	UpdateDisplays updateDisplays(&fs, &config, &can, &displays);
	ConfigureDisplays configureDisplays(&can, &config, &displays);

	// StateMachine
	StateMachine stateMachine(
		&startupState,
		{
			// Source: Startup
			{.from = &startupState, .signal = STATE_EVENT_ID::FIRST_STARTUP, .to = &configureDisplays},
			{.from = &startupState, .signal = STATE_EVENT_ID::UPDATE_AVAILABLE, .to = &updateDisplays},
			{.from = &startupState, .signal = STATE_EVENT_ID::STARTUP_FINISHED, .to = &operationState},

			// Source: UpdateDisplays
			{.from = &updateDisplays, .signal = STATE_EVENT_ID::UPDATE_FAILED, .to = &operationState},

			// SourceL ConfigureDisplays
			{.from = &configureDisplays, .signal = STATE_EVENT_ID::CONFIGURATION_FINISHED, .to = &operationState},
		});
	stateMachine.start();

	vTaskSuspend(nullptr);
}
