#ifndef FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_GUI
#define FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_GUI

/* --- Includes --- */

// Project includes
#include "FileManager/FileManager.h"

// espidf includes
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "esp_lcd_gc9a01.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "esp_timer.h"

// lvgl includes
#include "lvgl.h"

/* --- Defines & Macros --- */
#define GUI_LCD_SPI_HOST SPI2_HOST
#define GUI_LCD_RES 240
#define GUI_LCD_Bits _PER_PIXEL(16)

#define GUI_GPIO_LCD1_CS GPIO_NUM_39
#define GUI_GPIO_LCD2_CS GPIO_NUM_40
#define GUI_GPIO_LCD3_CS GPIO_NUM_41
#define GUI_GPIO_LCD_PCLK GPIO_NUM_42
#define GUI_GPIO_LCD_DATA0 GPIO_NUM_38
#define GUI_GPIO_LCD_RST GPIO_NUM_43
#define GUI_GPIO_LCD_DC GPIO_NUM_44

/* --- Variables, Typedefs etc. --- */

/* --- Imported Variables, Typedefs etc. --- */

/* --- Global variables and function (headers) --- */

//! \brief Initializes the GUI system
//! \retval A boolean indicating if the init was successful
bool guiInit(void);

//! \brief De-Initializes the GUI system. Mainly used so clang does stop
//! annoying with leaked memory.
void guiDeInit(void);

//! \brief Activates or disables the right blinker visually
//! \param active If true the blinker is shown
void guiSetRightBlinkerActive(const bool active);

//! \brief Activates or disables the left blinker visually
//! \param active If true the blinker is shown
void guiSetLeftBlinkerActive(const bool active);

//! \brief Updates the oil pressure
//! \param pressure Boolean indicating if there is oil pressure or not
//! \note Param is passed as void* as it's a cb function used by the SensorManager
void IRAM_ATTR guiSetOilPressure(void *pressure);

//! \brief Updates the fuel level percentage
//! \param percent 0 - 100 how much is the tank filled
//! \note Param is passed as void* as it's a cb function used by the SensorManager
void IRAM_ATTR guiSetFuelLevelPercent(void *percent);

//! \brief Updates the fuel level litres
//! \param litres Float indicating how many litres there are left in the tank
//! \note Param is passed as void* as it's a cb function used by the SensorManager
void IRAM_ATTR guiSetFuelLevelLitre(void *litres);

//! \brief Updates the water temperature
//! \param temp The temperature in Celsius as float
//! \note Param is passed as void* as it's a cb function used by the SensorManager
void IRAM_ATTR guiSetWaterTemperature(void *temp);

//! \brief Updates the speed
//! \param speed The speed as Integer
//! \note Param is passed as void* as it's a cb function used by the SensorManager
void IRAM_ATTR guiSetSpeed(void *speed);

//! \brief Updates the RPM
//! \param rpm The RPM as Integer
//! \note Param is passed as void* as it's a cb function used by the SensorManager
void IRAM_ATTR guiSetRpm(void *rpm);

#endif// FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_GUI
