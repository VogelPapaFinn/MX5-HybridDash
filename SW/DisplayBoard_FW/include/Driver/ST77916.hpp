#pragma once

// C++ includes
#include <atomic>
#include <bits/shared_ptr_atomic.h>

// espidf includes
#include "driver/ledc.h"
#include "driver/spi_common.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_types.h"
#include "lvgl.h"

/*
 *	class
 */
class ST77916
{
public:
	ST77916();

	void setLvglDisplay(lv_display_t* p_display);

	static void setBacklightLevel(uint8_t percent);

	void drawBitmap(const lv_area_t* p_area, uint8_t* p_pxMap);

	void setRotated(const bool& rotated) const;

private:
	/*
	 *	Private Struct
	 */
	struct DrawData
	{
		lv_area_t area;
		uint8_t* pixelData = nullptr;
		bool valid = false;
	};

	/*
	 *	Private Functions
	 */
	bool initSpiBus();
	bool initPanelIo();
	bool initLcdPanel();

	bool setupBacklight();

	bool setupTeGpio();

	/*
	 *	Private Static Functions
	 */
	//! \brief This ISR is triggered when the physical display is ready to accept new data.
	//!
	//! It then notifies an internal task which moves the new data into the display buffer
	static IRAM_ATTR void teGpioIsr(void* p_arg);

	static IRAM_ATTR void flushToDisplayTask(void* p_param);

	static IRAM_ATTR bool onFrameDrawn(esp_lcd_panel_io_handle_t p_panelIo, esp_lcd_panel_io_event_data_t* p_edata,
									   void* p_userCtx);

	/*
	 *	Private Variables
	 */
	bool initialized_ = false;

	// Display LCD Stuff
	esp_lcd_panel_io_handle_t ioHandle_ = nullptr;
	esp_lcd_panel_handle_t panelHandle_ = nullptr;

	lv_display_t* lvDisplay_ = nullptr;

	std::atomic<bool> isDrawing_{false};
	SemaphoreHandle_t spiMutex_ = nullptr;

	// TE GPIO Stuff
	DrawData drawData_;

	TaskHandle_t drawToDisplayTaskHandle_ = nullptr;
};
