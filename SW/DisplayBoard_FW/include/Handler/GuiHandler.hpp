#pragma once

// Project includes
#include "Can.hpp"
#include "Driver/ST77916.hpp"

// espidf includes
#include "esp_event_base.h"
#include "lvgl.h"

class GuiHandler
{
public:
	/*
	 *	Public enum
	 */
	typedef enum
	{
		UNKOWN = -1,
		TEMPERATURE,
		SPEED,
		RPM
	} SCREEN;

	/*
	 *	Public Functions
	 */
	GuiHandler();

	bool init();

	void loadScreen(SCREEN screen);

	void setRotation(bool rotation);

	void setLeftIndicatorActive(const bool& active) const;

	void setRightIndicatorActive(const bool& active) const;

	void setSpeed(const uint8_t& speed) const;

	void setRpm(const uint16_t& rpm) const;

	void setOilPressure(const bool& active) const;

	void setFuelLevel(const uint8_t& fuelLevelPercent) const;

	void setWaterTemperature(const uint8_t& temperature) const;

private:
	/*
	 *	Private Functions
	 */
	// LVGL
	bool initLvgl();
	bool initFrameBuffers();
	void registerCallbacks();
	bool startLvglUpdateTask();

	/*
	 *	Private Static Functions
	 */
	//! \note lv_display_flush_ready is called by the ST77916 driver as the physical display notifies when the buffer
	//! can be reused!
	IRAM_ATTR static void flushToDisplay(lv_display_t* p_display, const lv_area_t* p_area, uint8_t* p_pxMap);

	IRAM_ATTR static void updateLvgl(void* p_params);

	/*
	 *	Private Variables
	 */
	ST77916 st77916_;

	lv_display_t* display_ = nullptr;

	uint16_t* frameBuffer1_ = nullptr;
	uint16_t* frameBuffer2_ = nullptr;

	SCREEN currentScreen_ = UNKOWN;

	SemaphoreHandle_t guiMutex_;

	TaskHandle_t lvglUpdateTask_;
};
