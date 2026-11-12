// Project includes
#include "Can.hpp"
#include "Display.hpp"
#include "Filesystem.hpp"
#include "FilesystemLocations.hpp"
#include "KLine/KLine.hpp"
#include "SdCard.hpp"
#include "Sensors/HwSensorReader.hpp"
#include "SerialConsole/SerialConsole.hpp"
#include "StateMachine/StateEvents.hpp"
#include "StateMachine/StateMachine.hpp"
#include "StateMachine/States/ConfigureDisplays.hpp"
#include "StateMachine/States/Operation.hpp"
#include "StateMachine/States/Startup.hpp"
#include "StateMachine/States/UpdateDisplays.hpp"

// espidf includes
#include <esp_adc/adc_oneshot.h>
#include <esp_log.h>
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

constexpr gpio_num_t SDCARD_GPIO_CLK = GPIO_NUM_16;
constexpr gpio_num_t SDCARD_GPIO_CMD = GPIO_NUM_17;
constexpr gpio_num_t SDCARD_GPIO_D0 = GPIO_NUM_5;
constexpr gpio_num_t SDCARD_GPIO_D1 = GPIO_NUM_4;
constexpr gpio_num_t SDCARD_GPIO_D2 = GPIO_NUM_8;
constexpr gpio_num_t SDCARD_GPIO_D3 = GPIO_NUM_18;

constexpr adc_oneshot_unit_init_cfg_t ADC1_UNIT_CONFIG = {.unit_id = ADC_UNIT_1, .ulp_mode = ADC_ULP_MODE_DISABLE};

constexpr std::string SDCARD_BASE_PATH = "/sdcard";
constexpr uint8_t SDCARD_MAX_OPEN_FILES = 5;
constexpr std::string DATA_BASE_PATH = "/data";
constexpr std::string DATA_PARTITION_LABEL = "data";
constexpr std::string CONFIG_BASE_PATH = "/config";
constexpr std::string CONFIG_PARTITION_LABEL = "config";
constexpr std::string LOG_BASE_PATH = "/log";
constexpr std::string LOG_PARTITION_LABEL = "log";

/*
 *	Helper functions
 */
void initNvs()
{
	if (nvs_flash_init() != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initiate NVS Flash");
	}
}

void startEspEventLoop()
{
	if (esp_event_loop_create_default() == ESP_OK) {
		return;
	}

	ESP_LOGE(TAG, "Error creating default event loop. Rebooting...");
	esp_restart();
}

void initAdc1(adc_oneshot_unit_handle_t* p_adc1)
{
	if (adc_oneshot_new_unit(&ADC1_UNIT_CONFIG, p_adc1) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize ADC1");
	}
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

	initNvs();

	startEspEventLoop();

	// Drivers
	SdCard sdCard(SDCARD_GPIO_CLK, SDCARD_GPIO_CMD, SDCARD_GPIO_D0, SDCARD_GPIO_D1, SDCARD_GPIO_D2, SDCARD_GPIO_D3);
	Filesystem fs({
		{.id = FsLocation::SD_CARD,
		 .fsType = Filesystem::FAT_FS,
		 .basePath = SDCARD_BASE_PATH,
		 .sdCard = &sdCard,
		 .maxOpenFiles = SDCARD_MAX_OPEN_FILES},
		{.id = FsLocation::CONFIG_PARTITION,
		 .basePath = CONFIG_BASE_PATH,
		 .partitionLabel = CONFIG_PARTITION_LABEL,
		 .formatIfMountFails = true},
		{.id = FsLocation::DATA_PARTITION,
		 .basePath = DATA_BASE_PATH,
		 .partitionLabel = DATA_PARTITION_LABEL,
		 .formatIfMountFails = true},
		{.id = FsLocation::LOG_PARTITION,
		 .basePath = LOG_BASE_PATH,
		 .partitionLabel = LOG_PARTITION_LABEL,
		 .formatIfMountFails = true},
	});
	KLine kline;
	adc_oneshot_unit_handle_t adc1 = nullptr;
	Config config(&fs, FsLocation::CONFIG_PARTITION);
	Can can(GPIO_CAN_RX, GPIO_CAN_TX);
	WifiHost wifiHost;
	std::vector<Display> displays = {Display(GPIO_DISPLAY2, CAN_MASTER_ID + 2, 1, false),
									 Display(GPIO_DISPLAY1, CAN_MASTER_ID + 1, 0, true),
									 Display(GPIO_DISPLAY3, CAN_MASTER_ID + 3, 2, false)};
	LogFileServer logFileServer(&fs, &config, &wifiHost);
	SerialConsole serialConsole(&kline);

	initAdc1(&adc1);

	// Object which holds all sensor values
	SensorValues sensorValues;
	SensorLogger sensorLogger(&fs, &sensorValues);
	SensorBroadcaster sensorBroadcaster(&can, &sensorValues);
	HwSensorReader hwSensorReader(adc1, &sensorValues,
								  [&sensorBroadcaster]() { sensorBroadcaster.broadcastIndicators(); });

	// States
	Startup startupState(&fs, &config, &can, &displays);
	Operation operationState(&hwSensorReader, &sensorLogger, &sensorBroadcaster, &kline, &logFileServer, &sensorValues,
							 &serialConsole);
	UpdateDisplays updateDisplays(&fs, &config, &can, &wifiHost, &displays);
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
