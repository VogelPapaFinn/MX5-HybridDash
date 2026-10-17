// Project includes
#include "Can.hpp"
#include "Driver/Display.hpp"
#include "Driver/KLine.hpp"
#include "Events.hpp"
#include "Filesystem.hpp"
#include "Sensor/SensorHandler.hpp"
#include "State/Startup.hpp"
#include "WebInterface/WebInterface.hpp"
#include "WifiHost.hpp"

// espidf includes
#include <freertos/FreeRTOS.h>
#include "esp_adc/adc_oneshot.h"

//! \brief Main application tag.
constexpr auto TAG = "main";

//! \brief GPIO pin number for CAN receiver.
constexpr gpio_num_t GPIO_CAN_RX = GPIO_NUM_41;

//! \brief GPIO pin number for CAN transmitter.
constexpr gpio_num_t GPIO_CAN_TX = GPIO_NUM_40;

//! \brief Configuration file name.

constexpr gpio_num_t GPIO_DISPLAY1 = GPIO_NUM_13;
constexpr gpio_num_t GPIO_DISPLAY2 = GPIO_NUM_14;
constexpr gpio_num_t GPIO_DISPLAY3 = GPIO_NUM_21;

/*
 *	Private Static Variables
 */
//! \brief Current application state.
static std::shared_ptr<State> g_currentState;
static Filesystem g_fs;
static KLine g_kline;
static adc_oneshot_unit_handle_t g_adc1 = nullptr;
static Config* g_config = nullptr;
static Can g_can(GPIO_CAN_RX, GPIO_CAN_TX);
static std::vector<Display> g_displays = {Display(&g_can, GPIO_DISPLAY1, CAN_MASTER_ID + 1, 1, false),
										  Display(&g_can, GPIO_DISPLAY2, CAN_MASTER_ID + 2, 0, true),
										  Display(&g_can, GPIO_DISPLAY3, CAN_MASTER_ID + 3, 2, false)};
static WifiHost g_wifi;
static WebInterface g_webinterface(&g_fs);

/*
 *	Private Header Functions
 */
static void printLogHeader()
{
	ESP_LOGI(TAG, "--- --- --- --- --- --- ---");
	ESP_LOGI(TAG, "Startup");
}

/*
 *	main function
 */
extern "C" void app_main(void)
{
	// NEEDED FOR DEBUGGING
	vTaskDelay(pdMS_TO_TICKS(100));

	printLogHeader();

	// Handlers
	SensorHandler sensorHandler(g_adc1);

	// States
	Startup startupState(&g_fs, g_config, &g_can, g_adc1);

	while (true) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
