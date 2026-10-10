#include "../include/Gui.hpp"

// C++ includes
#include <string>

// espidf includes
#include "display/lv_display_private.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

// ui includes
extern "C" {
#include "ui/screens.h"
}

/*
 *	constexpr
 */
constexpr auto TAG = "Gui";

constexpr uint16_t RESOLUTION = 360;
constexpr uint8_t BIT_DEPTH_B = 2;

constexpr uint32_t FRAME_BUFFER_SIZE_B = (RESOLUTION * RESOLUTION * BIT_DEPTH_B) / 4;

/*
 *	Private Mutex Class
 */
class GuiLock
{
public:
	GuiLock(SemaphoreHandle_t p_mutex) : mutex_(p_mutex) { xSemaphoreTakeRecursive(mutex_, portMAX_DELAY); }

	~GuiLock() { xSemaphoreGiveRecursive(mutex_); }

private:
	SemaphoreHandle_t mutex_;
};

/*
 *	Static functions
 */
static void staticLvglUpdateFunc(void* p_params)
{
	if (p_params == nullptr) {
		vTaskDelete(nullptr);
		return;
	}

	Gui* gui = static_cast<Gui*>(p_params);
	const auto mutex = gui->getGuiMutex();

	while (true) {
		{
			GuiLock lock(mutex);

			lv_timer_handler();
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

/*
 *	Public Function Implementations
 */
Gui::Gui(SystemContext* p_sysCon)
{
	sysCon_ = p_sysCon;

	/*
	 *	Initiate lvgl
	 */
	lv_init();

	/*
	 * Init display
	 */
	display_ = lv_display_create(RESOLUTION, RESOLUTION);
	lv_display_set_user_data(display_, this);
	st77916_.setLvglDisplay(display_);

	/*
	 * Create frame buffers
	 */
	frameBuffer1_ = static_cast<uint16_t*>(malloc(FRAME_BUFFER_SIZE_B));
	frameBuffer2_ = static_cast<uint16_t*>(malloc(FRAME_BUFFER_SIZE_B));
	if (frameBuffer1_ == nullptr || frameBuffer2_ == nullptr) {
		ESP_LOGE(TAG, "Failed to allocate memory for the frame buffers");
		return;
	}
	memset(frameBuffer1_, 0, FRAME_BUFFER_SIZE_B);
	memset(frameBuffer2_, 0, FRAME_BUFFER_SIZE_B);

	/*
	 * Set frame buffers
	 */
	lv_display_set_buffers(display_, frameBuffer1_, frameBuffer2_, FRAME_BUFFER_SIZE_B, LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_display_set_color_format(display_, LV_COLOR_FORMAT_RGB565);
	lv_obj_set_style_bg_color(lv_display_get_screen_active(display_), lv_color_hex(0x000000), // Black
							  LV_PART_MAIN);

	// Create mutex to prevent racing conditions
	guiMutex_ = xSemaphoreCreateRecursiveMutex();

	/*
	 * Register callbacks
	 */
	lv_display_set_flush_cb(display_,
							[](lv_display_t* p_display, const lv_area_t* p_area, uint8_t* p_pxMap)
							{
								/*
								 *	Preparations
								 */
								if (p_display == nullptr || lv_display_get_user_data(p_display) == nullptr ||
									p_area == nullptr || p_pxMap == nullptr) {
									return;
								}

								/*
								 *	Get the GUI instance
								 */
								auto gui = static_cast<Gui*>(lv_display_get_user_data(p_display));

								/*
								 *	Call the flush function
								 */
								gui->flushToDisplay(p_area, p_pxMap);
							});
	lv_tick_set_cb(xTaskGetTickCount);

	/*
	 * Create tasks
	 */
	if (xTaskCreate(staticLvglUpdateFunc, "lvglUpdateTaskFunc", 10240, this, 3, &lvglUpdateTask_) != pdPASS) {
		ESP_LOGE(TAG, "Failed to create LVGL update task");
		return;
	}

	/*
	 *	Register to necessary events
	 */
	registerToEvents();
}

SemaphoreHandle_t Gui::getGuiMutex() const { return guiMutex_; }

/*
 *	Private Function Implementations
 */
void Gui::registerToEvents()
{
	/*
	 *	Load Screen
	 */
	eventHandlers_.push_back(std::make_tuple(SYSTEM_EVENT_BASE, LOAD_SCREEN, esp_event_handler_instance_t()));
	esp_event_handler_instance_register(
		SYSTEM_EVENT_BASE, LOAD_SCREEN,
		[](void* p_gui, esp_event_base_t, int32_t, void* p_screen)
		{
			/*
			 *	Get the gui ptr
			 */
			if (p_gui == nullptr) {
				return;
			}

			// Convert it
			Gui* gui = static_cast<Gui*>(p_gui);

			/*
			 *	Get the screen
			 */
			if (p_screen == nullptr) {
				return;
			}

			const auto screen = static_cast<uint8_t*>(p_screen);

			/*
			 *	Load the correct screen
			 */
			gui->loadScreen(static_cast<SCREEN>(*screen));
		},
		this, &get<2>(eventHandlers_.back()));

	/*
	 *	Turn on the display
	 */
	eventHandlers_.push_back(std::make_tuple(SYSTEM_EVENT_BASE, TURN_ON, esp_event_handler_instance_t()));
	esp_event_handler_instance_register(
		SYSTEM_EVENT_BASE, TURN_ON,
		[](void* p_gui, esp_event_base_t, int32_t, void*)
		{
			/*
			 *	Get the gui ptr
			 */
			if (p_gui == nullptr) {
				return;
			}

			// Convert it
			Gui* gui = static_cast<Gui*>(p_gui);

			/*
			 *	Turn the display on
			 */
			gui->st77916_.setBacklightLevel(100);
		},
		this, &get<2>(eventHandlers_.back()));

	/*
	 *	New Sensor Data
	 */
	eventHandlers_.push_back(std::make_tuple(SYSTEM_EVENT_BASE, NEW_SENSOR_DATA, esp_event_handler_instance_t()));
	esp_event_handler_instance_register(
		SYSTEM_EVENT_BASE, NEW_SENSOR_DATA,
		[](void* p_gui, esp_event_base_t, int32_t, void* p_sensorData)
		{
			/*
			 *	Get the gui ptr
			 */
			if (p_gui == nullptr) {
				return;
			}

			// Convert it
			Gui* gui = static_cast<Gui*>(p_gui);

			/*
			 *	Get the sensor data struct
			 */
			if (p_sensorData == nullptr) {
				return;
			}

			const auto sensorData = static_cast<SensorData*>(p_sensorData);

			/*
			 *	Update the visible sensors
			 */
			switch (gui->currentScreen_) {
				case TEMPERATURE:
					{
						gui->setFuelLevel(sensorData->fuelLevel);
						gui->setOilPressure(sensorData->oilPressure);
						gui->setWaterTemperature(sensorData->waterTemperature);
					}
					break;

				case SPEED:
					{
						gui->setSpeed(sensorData->speed);
						gui->setRightIndicatorActive(sensorData->rightIndicatorActive);
					}
					break;

				case RPM:
					{
						gui->setRpm(sensorData->rpm);
						gui->setLeftIndicatorActive(sensorData->leftIndicatorActive);
					}
					break;
				default:;
			}
		},
		this, &get<2>(eventHandlers_.back()));
}

void Gui::loadScreen(const SCREEN screen)
{
	/*
	 *	Preparations
	 */
	// Save the new screen
	currentScreen_ = screen;

	// Acquire GUI lock
	GuiLock lock(guiMutex_);

	/*
	 *	Load correct screen
	 */
	switch (screen) {
		case TEMPERATURE:
			create_screen_temperature();
			lv_scr_load(objects.temperature);
			break;
		case SPEED:
			create_screen_speed();
			lv_scr_load(objects.speed);
			break;
		case RPM:
			create_screen_rpm();
			lv_scr_load(objects.rpm);
			break;
		default:
			break;
	}
}

void Gui::flushToDisplay(const lv_area_t* p_area, uint8_t* p_pxMap)
{
	/*
	 *	Preparations
	 */
	// Acquire GUI lock
	GuiLock lock(guiMutex_);

	/*
	 *	Swap each lower and upper byte to display the correct colors
	 */
	const uint32_t amountOfPixels = (p_area->x2 - p_area->x1 + 1) * (p_area->y2 - p_area->y1 + 1);
	lv_draw_sw_rgb565_swap(p_pxMap, amountOfPixels);

	/*
	 *	Draw the bitmap onto the physical screen
	 */
	st77916_.drawBitmap(p_area, p_pxMap);

	// lv_display_flush_ready is called by the ST77916 driver as the physical display
	// notifies when the buffer can be reused!
}

/*
 *	Private Setter-Function Implementations
 */
void Gui::setLeftIndicatorActive(const bool& active) const
{
	if (objects.left_indicator == nullptr) {
		return;
	}

	GuiLock lock(guiMutex_);

	if (active) {
		lv_obj_set_style_image_opa(objects.left_indicator, LV_OPA_100, LV_PART_MAIN);
	}
	else {
		lv_obj_set_style_image_opa(objects.left_indicator, LV_OPA_20, LV_PART_MAIN);
	}
}

void Gui::setRightIndicatorActive(const bool& active) const
{
	if (objects.right_indicator == nullptr) {
		return;
	}

	GuiLock lock(guiMutex_);

	if (active) {
		lv_obj_set_style_image_opa(objects.right_indicator, LV_OPA_100, LV_PART_MAIN);
	}
	else {
		lv_obj_set_style_image_opa(objects.right_indicator, LV_OPA_20, LV_PART_MAIN);
	}
}

void Gui::setSpeed(const uint8_t& speed) const
{
	if (objects.speed_label == nullptr) {
		return;
	}

	GuiLock lock(guiMutex_);

	lv_label_set_text(objects.speed_label, std::to_string(speed).c_str());
}

void Gui::setRpm(const uint16_t& rpm) const
{
	if (objects.rpm_label == nullptr) {
		return;
	}

	GuiLock lock(guiMutex_);

	lv_label_set_text(objects.rpm_label, std::to_string(rpm).c_str());
}

void Gui::setOilPressure(const bool& active) const
{
	if (objects.oil_can == nullptr) {
		return;
	}

	GuiLock lock(guiMutex_);

	if (active) {
		lv_obj_set_style_image_opa(objects.oil_can, LV_OPA_0, LV_PART_MAIN);
	}
	else {
		lv_obj_set_style_image_opa(objects.oil_can, LV_OPA_100, LV_PART_MAIN);
	}
}

void Gui::setFuelLevel(const uint8_t& fuelLevelPercent) const
{
	if (objects.percent_label == nullptr) {
		return;
	}

	// Check if it's the same value
	static uint8_t s_lastFuelLevelValue = 0;
	if (fuelLevelPercent == s_lastFuelLevelValue) {
		return;
	}
	s_lastFuelLevelValue = fuelLevelPercent;

	GuiLock lock(guiMutex_);

	lv_label_set_text(objects.percent_label, (std::to_string(fuelLevelPercent) + "%").c_str());

	const std::vector<lv_obj_t*> arcs = {objects.arc0, objects.arc1, objects.arc2, objects.arc3, objects.arc4,
										 objects.arc5, objects.arc6, objects.arc7, objects.arc8, objects.arc9};
	for (uint8_t i = 0; i < fuelLevelPercent / 10; i++) {
		lv_obj_set_style_arc_opa(arcs[i], LV_OPA_100, LV_PART_MAIN);
	}
	for (uint8_t i = fuelLevelPercent / 10; i < 10; i++) {
		lv_obj_set_style_arc_opa(arcs[i], LV_OPA_20, LV_PART_MAIN);
	}
}

void Gui::setWaterTemperature(const int8_t& temperature) const
{
	if (objects.temperature_label == nullptr) {
		return;
	}

	GuiLock lock(guiMutex_);

	lv_label_set_text(objects.temperature_label, std::to_string(temperature).c_str());
}
