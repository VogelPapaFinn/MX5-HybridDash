/* --- Includes --- */
#include "Core/Core.h"

/* --- Private Defines & Macros --- */

/* --- Private Variables, Typedefs etc. --- */

// Task handlers
TaskHandle_t taskOilPressureHandler_ = NULL;
TaskHandle_t taskFuelLevelHandler_ = NULL;
TaskHandle_t taskWaterTemperatureHandler_ = NULL;
TaskHandle_t taskInternalTemperatureHandler_ = NULL;
TaskHandle_t taskSpeedHandler_ = NULL;
TaskHandle_t taskRpmHandler_ = NULL;

/* --- Tasks --- */

//! \brief Task, which updates the oil pressure periodically
void IRAM_ATTR taskUpdateOilPressure(void *params) {
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        // Update the oil pressure
        sensorManagerUpdateOilPressure();

        // Wait X milliseconds
        vTaskDelay(pdMS_TO_TICKS(OIL_PRESSURE_UPDATE_INTERVAL_MS));
    }
}

//! \brief Task, which updates the fuel level periodically
void IRAM_ATTR taskUpdateFuelLevel(void *params) {
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        // Update the fuel level
        sensorManagerUpdateFuelLevel();

        // Wait X milliseconds
        vTaskDelay(pdMS_TO_TICKS(FUEL_LEVEL_UPDATE_INTERVAL_MS));
    }
}

//! \brief Task, which updates the water temp periodically
void IRAM_ATTR taskUpdateWaterTemperature(void *params) {
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        // Update the water temperature
        sensorManagerUpdateWaterTemperature();

        // Wait X milliseconds
        vTaskDelay(pdMS_TO_TICKS(WATER_TEMPERATURE_UPDATE_INTERVAL_MS));
    }
}

//! \brief Task, which updates the internal temp periodically
void IRAM_ATTR taskUpdateInternalTemperature(void *params) {
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        //Update the internal temperature
        sensorManagerUpdateInternalTemperature();

        // Wait X milliseconds
        vTaskDelay(pdMS_TO_TICKS(INTERNAL_TEMPERATURE_UPDATE_INTERVAL_MS));
    }
}

//! \brief Task, which updates the speed periodically
void IRAM_ATTR taskUpdateSpeed(void *params) {
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        // Update the speed
        sensorManagerUpdateSpeed();

        // Wait X milliseconds
        vTaskDelay(pdMS_TO_TICKS(SPEED_UPDATE_INTERVAL_MS));

        //loggerInfo("Updating Speed!");
    }
}

//! \brief Task, which updates the RPM periodically
void IRAM_ATTR taskUpdateRpm(void *params) {
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        // Update the RPM
        sensorManagerUpdateRPM();

        // Wait X milliseconds
        vTaskDelay(pdMS_TO_TICKS(RPM_UPDATE_INTERVAL_MS));
    }
}

/* --- Function implementations --- */

bool coreInit(void) {
    bool success = true;

    // Register the callbacks
    sensorManagerRegisterCallback(SENSOR_OIL_PRESSURE, guiSetOilPressure);
    sensorManagerRegisterCallback(SENSOR_FUEL_LEVEL_PERCENT, guiSetFuelLevelPercent);
    sensorManagerRegisterCallback(SENSOR_FUEL_LEVEL_LITRE, guiSetFuelLevelLitre);
    sensorManagerRegisterCallback(SENSOR_WATER_TEMPERATURE, guiSetWaterTemperature);
    sensorManagerRegisterCallback(SENSOR_SPEED, guiSetSpeed);
    sensorManagerRegisterCallback(SENSOR_RPM, guiSetRpm);

    // Start the update oil pressure task
    success &= xTaskCreate(taskUpdateOilPressure, "taskUpdateOilPressure", 8196, NULL, OIL_PRESSURE_PRIORITY_LEVEL, &taskOilPressureHandler_);

    // Start the update fuel level task
    success &= xTaskCreate(taskUpdateFuelLevel, "taskUpdateFuelLevel", 8196, NULL, FUEL_LEVEL_PRIORITY_LEVEL, &taskFuelLevelHandler_);

    // Start the update water temperature task
    success &= xTaskCreate(taskUpdateWaterTemperature, "taskUpdateWaterTemperature", 8196, NULL, WATER_TEMPERATURE_PRIORITY_LEVEL, &taskWaterTemperatureHandler_);

    // Start the update internal temperature task
    success &= xTaskCreate(taskUpdateInternalTemperature, "taskUpdateInternalTemperature", 8196, NULL, INTERNAL_TEMPERATURE_PRIORITY_LEVEL, &taskInternalTemperatureHandler_);

    // Start the update speed task
    success &= xTaskCreate(taskUpdateSpeed, "taskUpdateSpeed", 8196 * 2, NULL, SPEED_PRIORITY_LEVEL, &taskSpeedHandler_);

    // Start the update rpm task
    success &= xTaskCreate(taskUpdateRpm, "taskUpdateRpm", 8196, NULL, RPM_PRIORITY_LEVEL, &taskRpmHandler_);

    // Did everything work?
    if (!success) {
        // Logging
        loggerCritical("Couldn't create one or more update tasks!");

        return false;
    }

    // Everything worked
    return true;
}
