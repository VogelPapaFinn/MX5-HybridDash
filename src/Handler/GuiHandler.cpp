#include "Handler/GuiHandler.hpp"

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
 *	Public Function Implementations
 */
GuiHandler::GuiHandler() { guiMutex_ = xSemaphoreCreateRecursiveMutex(); }

bool GuiHandler::init()
{
	if (!initLvgl()) {
		return false;
	}

	if (!initFrameBuffers()) {
		return false;
	}

	registerCallbacks();

	if (!startLvglUpdateTask()) {
		return false;
	}

	return true;
}

void GuiHandler::loadScreen(const SCREEN screen)
{
	currentScreen_ = screen;

	GuiLock lock(guiMutex_);

	// Create and load screen
	switch (screen) {
		case TEMPERATURE: {
			create_screen_temperature();
			lv_scr_load(objects.temperature);
		} break;

		case SPEED: {
			create_screen_speed();
			lv_scr_load(objects.speed);
		} break;

		case RPM: {
			create_screen_rpm();
			lv_scr_load(objects.rpm);
		} break;

		default: {
		} break;
	}
}

void GuiHandler::setLeftIndicatorActive(const bool& active) const
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

void GuiHandler::setRightIndicatorActive(const bool& active) const
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

void GuiHandler::setSpeed(const uint8_t& speed) const
{
	if (objects.speed_label == nullptr) {
		return;
	}

	GuiLock lock(guiMutex_);

	lv_label_set_text(objects.speed_label, std::to_string(speed).c_str());
}

void GuiHandler::setRpm(const uint16_t& rpm) const
{
	if (objects.rpm_label == nullptr) {
		return;
	}

	GuiLock lock(guiMutex_);

	lv_label_set_text(objects.rpm_label, std::to_string(rpm).c_str());
}

void GuiHandler::setOilPressure(const bool& active) const
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

void GuiHandler::setFuelLevel(const uint8_t& fuelLevelPercent) const
{
	if (objects.percent_label == nullptr) {
		return;
	}

	// Ignore if the value didnt change
	static uint8_t s_lastFuelLevelValue = 0;
	if (fuelLevelPercent == s_lastFuelLevelValue) {
		return;
	}

	s_lastFuelLevelValue = fuelLevelPercent;

	GuiLock lock(guiMutex_);

	// Update the label
	lv_label_set_text(objects.percent_label, (std::to_string(fuelLevelPercent) + "%").c_str());

	// Update the indicator on the left side
	const std::vector<lv_obj_t*> arcs = {objects.arc0, objects.arc1, objects.arc2, objects.arc3, objects.arc4,
										 objects.arc5, objects.arc6, objects.arc7, objects.arc8, objects.arc9};
	for (uint8_t i = 0; i < fuelLevelPercent / 10; i++) {
		lv_obj_set_style_arc_opa(arcs[i], LV_OPA_100, LV_PART_MAIN);
	}
	for (uint8_t i = fuelLevelPercent / 10; i < 10; i++) {
		lv_obj_set_style_arc_opa(arcs[i], LV_OPA_20, LV_PART_MAIN);
	}
}

void GuiHandler::setWaterTemperature(const uint8_t& temperature) const
{
	if (objects.temperature_label == nullptr) {
		return;
	}

	GuiLock lock(guiMutex_);

	lv_label_set_text(objects.temperature_label, std::to_string(temperature).c_str());
}

/*
 *	Private Function Implementations
 */
bool GuiHandler::initLvgl()
{
	lv_init();

	// Init display
	display_ = lv_display_create(RESOLUTION, RESOLUTION);
	lv_display_set_user_data(display_, this);
	st77916_.setLvglDisplay(display_);

	return display_ != nullptr;
}

bool GuiHandler::initFrameBuffers()
{
	// Create frame buffers
	frameBuffer1_ = static_cast<uint16_t*>(malloc(FRAME_BUFFER_SIZE_B));
	frameBuffer2_ = static_cast<uint16_t*>(malloc(FRAME_BUFFER_SIZE_B));
	if (frameBuffer1_ == nullptr || frameBuffer2_ == nullptr) {
		ESP_LOGE(TAG, "Failed to allocate memory for the frame buffers");
		return false;
	}

	// Clear buffers
	memset(frameBuffer1_, 0, FRAME_BUFFER_SIZE_B);
	memset(frameBuffer2_, 0, FRAME_BUFFER_SIZE_B);

	// Set framebuffers
	lv_display_set_buffers(display_, frameBuffer1_, frameBuffer2_, FRAME_BUFFER_SIZE_B, LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_display_set_color_format(display_, LV_COLOR_FORMAT_RGB565);
	lv_obj_set_style_bg_color(lv_display_get_screen_active(display_), lv_color_hex(0x000000), // Black
							  LV_PART_MAIN);

	return true;
}

void GuiHandler::registerCallbacks()
{
	lv_display_set_flush_cb(display_, flushToDisplay);
	lv_tick_set_cb(xTaskGetTickCount);
}

bool GuiHandler::startLvglUpdateTask()
{
	if (xTaskCreate(updateLvgl, "UpdateLvgl", 10240, this, 3, &lvglUpdateTask_) != pdPASS) {
		ESP_LOGE(TAG, "Failed to create LVGL update task");
		return false;
	}

	return true;
}

/*
 *	Private Static Function Implementations
 */
void GuiHandler::flushToDisplay(lv_display_t* p_display, const lv_area_t* p_area, uint8_t* p_pxMap)
{
	if (p_display == nullptr || lv_display_get_user_data(p_display) == nullptr || p_area == nullptr ||
		p_pxMap == nullptr) {
		return;
	}

	auto gui = static_cast<GuiHandler*>(lv_display_get_user_data(p_display));

	GuiLock lock(gui->guiMutex_);

	// Swap each lower and upper byte to display the correct colors
	const uint32_t amountOfPixels = (p_area->x2 - p_area->x1 + 1) * (p_area->y2 - p_area->y1 + 1);
	lv_draw_sw_rgb565_swap(p_pxMap, amountOfPixels);

	// Draw the bitmap onto the physical screen
	gui->st77916_.drawBitmap(p_area, p_pxMap);
}

void GuiHandler::updateLvgl(void* p_params)
{
	if (p_params == nullptr) {
		vTaskDelete(nullptr);
		return;
	}

	GuiHandler* gui = static_cast<GuiHandler*>(p_params);
	const auto mutex = gui->guiMutex_;

	while (true) {
		{
			GuiLock lock(mutex);

			lv_timer_handler();
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
