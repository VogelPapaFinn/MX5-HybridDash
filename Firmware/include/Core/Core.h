#ifndef FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_CORE
#define FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_CORE

/* --- Includes --- */

// Project includes
#include "GUI/GUI.h"
#include "Logger/Logger.h"
#include "SensorManager/SensorManager.h"

// C includes
#include <stdbool.h>

/* --- Defines & Macros --- */

// UPDATE INTERVALS
#define OIL_PRESSURE_UPDATE_INTERVAL_MS 10 * 1000       // 10s
#define FUEL_LEVEL_UPDATE_INTERVAL_MS 250               //12 * 1000// 120s
#define WATER_TEMPERATURE_UPDATE_INTERVAL_MS 5 * 1000   // 5s
#define INTERNAL_TEMPERATURE_UPDATE_INTERVAL_MS 5 * 1000// 5s
#define SPEED_UPDATE_INTERVAL_MS 250                    // 0.25s
#define RPM_UPDATE_INTERVAL_MS 250                      // 0.25s

// TASK PRIORITIES
#define OIL_PRESSURE_PRIORITY_LEVEL 0
#define FUEL_LEVEL_PRIORITY_LEVEL 2
#define WATER_TEMPERATURE_PRIORITY_LEVEL 1
#define INTERNAL_TEMPERATURE_PRIORITY_LEVEL 2
#define SPEED_PRIORITY_LEVEL 0
#define RPM_PRIORITY_LEVEL 0

/* --- Variables, Typedefs etc. --- */

/* --- Imported Variables, Typedefs etc. --- */

/* --- Global variables and function (headers) --- */

//! \brief Initializes the core
//! \retval bool Indicating if everything worked
bool coreInit(void);

#endif// FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_CORE
