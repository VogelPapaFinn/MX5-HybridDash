#ifndef FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_SENSORMANAGER
#define FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_SENSORMANAGER

/* --- Includes --- */
// C includes
#include <math.h>
#include <stdbool.h>

// Project includes
#include "Logger/Logger.h"

// espidf includes
#include <driver/gpio.h>
#include <driver/pulse_cnt.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_timer.h>

// freeRTOS includes
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

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
#define GPIO_SPEED GPIO_NUM_14
#define GPIO_RPM GPIO_NUM_21

// ADC CHANNELS
#define ADC_CHANNEL_OIL_PRESSURE ADC_CHANNEL_1
#define ADC_CHANNEL_FUEL_LEVEL ADC_CHANNEL_0
#define ADC_CHANNEL_WATER_TEMPERATURE ADC_CHANNEL_2
#define ADC_CHANNEL_INT_TEMPERATURE ADC_CHANNEL_6

// OIL PRESSURE THRESHOLDS
#define OIL_LOWER_VOLTAGE_THRESHOLD 65 // mV -> R2 ~= 5 Ohms
#define OIL_UPPER_VOLTAGE_THRESHOLD 255// mV -> R2 ~= 20 Ohms

// FUEL LEVEL CALCULATION STUFF
#define FUEL_LEVEL_OFFSET 5.0f
#define FUEL_LEVEL_TO_PERCENTAGE 115.0f// Divide the calculated resistance by this value to get the level in percent

/* --- Variables, Typedefs etc. --- */

typedef enum {
    SENSOR_OIL_PRESSURE,
    SENSOR_FUEL_LEVEL_PERCENT,
    SENSOR_FUEL_LEVEL_LITRE,
    SENSOR_WATER_TEMPERATURE,
    SENSOR_INTERNAL_TEMPERATURE,
    SENSOR_SPEED,
    SENSOR_RPM,
} SENSOR;

/* --- Imported Variables, Typedefs etc. --- */

/* --- Global variables and function (headers) --- */
//! \brief Initializes the SensorManager
//! \retval 0 - Initialization failed
//! \retval 1 - Initialization succeeded without errors
//! \retval 2 - Initialization succeeded with errors. See log
int sensorManagerInit(void);

//! \brief Registers a callback function which will be called once the specified value changes
//! (e.g. oil, fuel, water temp etc.)
//! \param sensorType The sensor the callback should be registered on
//! \param callback The callback function
void sensorManagerRegisterCallback(const SENSOR sensorType, void callback(void *val));

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

//! \brief Enables the speed ISR
//! \retval Boolean indicating if it worked
bool sensorManagerEnableSpeedISR(void);

//! \brief Disables the speed ISR
void sensorManagerDisableSpeedISR(void);

//! \brief Updates the speed
void sensorManagerUpdateSpeed(void);

//! \brief Returns the speed
//! \retval The speed as integer
int sensorManagerGetSpeed(void);

//! \brief Enables the rpm ISR
//! \retval Boolean indicating if it worked
bool sensorManagerEnableRpmISR(void);

//! \brief Disables the rpm ISR
void sensorManagerDisableRpmISR(void);

//! \brief Updates the rpm
void sensorManagerUpdateRPM(void);

//! \brief Returns the rpm
//! \retval The rpm as integer
int sensorManagerGetRPM(void);

//! \brief Updates the internal temperature
void sensorManagerUpdateInternalTemperature(void);

//! \brief Returns the internal temperature in degree Celsius
//! \retval The temperature as double
double sensorManagerGetInternalTemperature(void);

#endif// FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_SENSORMANAGER
