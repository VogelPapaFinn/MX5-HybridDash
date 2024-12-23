#ifndef FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_SENSORMANAGER
#define FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_SENSORMANAGER

/* --- Includes --- */
// C includes
#include <stdbool.h>

// Project includes
#include "Logger/Logger.h"

// espidf includes
#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>

/* --- Defines & Macros --- */

// Defines used for the voltage divider/ohmmeter to measure
// the oil pressure, fuel level and water temperature
#define OIL_FUEL_WATER_VOLTAGE_V 3.3
#define OIL_FUEL_R1 240
#define WATER_R1 3000

// GPIOs
#define GPIO_OIL_PRESSURE GPIO_NUM_12
#define GPIO_FUEL_LEVEL GPIO_NUM_11
#define GPIO_WATER_TEMPERATURE GPIO_NUM_13

// ADC CHANNELS
#define ADC_CHANNEL_OIL_PRESSURE ADC_CHANNEL_1
#define ADC_CHANNEL_FUEL_LEVEL ADC_CHANNEL_0
#define ADC_CHANNEL_WATER_TEMPERATURE ADC_CHANNEL_2

// OIL PRESSURE THRESHOLDS
#define OIL_LOWER_VOLTAGE_THRESHOLD 65 // mV - R2 ~= 5 Ohms
#define OIL_UPPER_VOLTAGE_THRESHOLD 255// mV - R2 ~= 20 Ohms

// FUEL LEVEL CALCULATION STUFF
#define FUEL_LEVEL_OFFSET 5.0f
#define FUEL_LEVEL_TO_PERCENTAGE 115.0f// Divide the calculated resistance by this value to get the level in percent

/* --- Variables, Typedefs etc. --- */

/* --- Imported Variables, Typedefs etc. --- */

/* --- Global variables and function (headers) --- */
//! \brief Initializes the SensorManager
//! \retval 0 - Initialization failed
//! \retval 1 - Initialization succeeded without errors
//! \retval 2 - Initialization succeeded with errors. See log
int sensorManagerInit(void);

//! \brief Registers a callback function which will be called once the specified value changes
//! (e.g. oil, fuel, water temp etc.)
//! TODO: Implement this function, which is probably needed for the GUI part.
void sensorManagerRegisterCallback(void);

//! \brief Checks if there is oil pressure
void sensorManagerUpdateOilPressure(void);

//! \brief Returns a boolean indicating if there is oil pressure or not
//! \retval Boolean
bool sensorManagerHasOilPressure(void);

//! \brief Updates the fuel level
void sensorManagerUpdateFuelLevel(void);

//! \brief Returns the fuel level in PERCENT
//! \retval The fuel level in PERCENT as int
int sensorManagerGetFuelLevel(void);

//! \brief Updates the water temperature
void sensorManagerUpdateWaterTemperature(void);

//! \brief Returns the water temperature in degree Celsius
//! \retval The water temperature as float
float sensorManagerGetWaterTemperature(void);

#endif// FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_SENSORMANAGER
