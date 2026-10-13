// Project includes
#include "Can.hpp"
#include "Driver/Display.hpp"
#include "Events.hpp"
#include "Filesystem.hpp"
#include "Handler/RegistrationHandler.hpp"
#include "State/Operation.hpp"
#include "State/Registration.hpp"

// espidf includes
#include <freertos/FreeRTOS.h>

//! \brief Main application tag.
constexpr auto TAG = "main";

//! \brief GPIO pin number for CAN receiver.
constexpr gpio_num_t GPIO_CAN_RX = GPIO_NUM_41;

//! \brief GPIO pin number for CAN transmitter.
constexpr gpio_num_t GPIO_CAN_TX = GPIO_NUM_40;

//! \brief Configuration file name.
constexpr auto CONFIG_NAME = "config.json";

//! \brief Default configuration file name.
constexpr auto DEFAULT_CONFIG_NAME = "default/config.json";

constexpr adc_oneshot_unit_init_cfg_t ADC1_UNIT_CONFIG = {.unit_id = ADC_UNIT_1, .ulp_mode = ADC_ULP_MODE_DISABLE};

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
static Config g_config(&g_fs);
static Can g_can(GPIO_CAN_RX, GPIO_CAN_TX);
static std::vector<Display> g_displays = {Display(&g_can, GPIO_DISPLAY1, CAN_MASTER_ID + 1, 1, false),
										  Display(&g_can, GPIO_DISPLAY2, CAN_MASTER_ID + 2, 0, true),
										  Display(&g_can, GPIO_DISPLAY3, CAN_MASTER_ID + 3, 2, false)};
static WifiHost g_wifi;
static WebInterface g_webinterface(&g_fs);

/*
 *	Helper functions
 */
//! \brief Registers necessary events for system initialization.
static void registerToEvents()
{
	/*
	 *	Registration Completed
	 */
	esp_event_handler_instance_register(
		SYSTEM_EVENT_BASE, REGISTRATION_COMPLETED,
		[](void*, esp_event_base_t, int32_t, void*)
		{
			/*
			 *	Enter the operation state
			 */
			g_currentState =
				std::make_shared<Operation>(&g_fs, g_adc1, &g_kline, &g_wifi, &g_config, &g_can, &g_displays);
			g_currentState->enter();
		},
		nullptr, nullptr);
}

//! \brief Creates and opens the configuration file.
static void createAndOpenConfigFile()
{
	/*
	 *	Create default config, if config doesnt exist
	 */
	if (!g_fs.doesFileExist(CONFIG_NAME, Filesystem::CONFIG_PARTITION)) {
		g_fs.createFile(CONFIG_NAME, Filesystem::CONFIG_PARTITION);

		if (!g_fs.doesFileExist(DEFAULT_CONFIG_NAME, Filesystem::CONFIG_PARTITION)) {
			Config defaultConfig(&g_fs);
			defaultConfig.open(DEFAULT_CONFIG_NAME);

			Config newConfig(&g_fs);
			newConfig.open(CONFIG_NAME);
			*newConfig.getJson() = *defaultConfig.getJson();

			newConfig.save();
		}
	}

	/*
	 *	Load the config file
	 */
	g_config.open(CONFIG_NAME);
	const auto& jsonConfig = g_config.getJson();
	if (jsonConfig != nullptr) {
		std::string str;
		serializeJsonPretty(*jsonConfig, str);
		ESP_LOGI(TAG, "%s", str.c_str());
	}
}

static void printLogHeader()
{
	ESP_LOGI(TAG, "--- --- --- --- --- --- ---");
	ESP_LOGI(TAG, "Startup");
}

static void startEventLoop()
{
	if (esp_event_loop_create_default() != ESP_OK) {
		ESP_LOGE(TAG, "Error creating default event loop. Rebooting...");
		esp_restart();
	}
}

/*
 *	main function
 */
extern "C" void app_main(void)
{
	// NEEDED FOR DEBUGGING
	vTaskDelay(pdMS_TO_TICKS(100));

	printLogHeader();
	startEventLoop();

	g_can.initialize();
	g_can.enable();

	// Registration Handler
	RegistrationHandler regHandler(&g_can, &g_displays);

	/*
	 *	Setup the ADC1
	 */
	if (adc_oneshot_new_unit(&ADC1_UNIT_CONFIG, &g_adc1) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize ADC1");
	}

	/*
	 *	Register the necessary events
	 */
	registerToEvents();

	/*
	 *	Ensure the config file exists and is loaded
	 */
	createAndOpenConfigFile();

	/*
	 *	Create and enter the registration state
	 */
	g_currentState = std::make_shared<Registration>(&g_can, &g_displays);
	g_currentState->enter();

	while (true) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
