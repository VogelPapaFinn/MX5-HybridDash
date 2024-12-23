/* Includes */

// Project
#include "FileManager/FileManager.h"
#include "Logger/Logger.h"

#include <SensorManager/SensorManager.h>

void app_main(void) {
    // Initialize FileManager
    const bool fileManagerInitResult = fileManagerInit();
    if (fileManagerInitResult) {
        // Logging
        loggerInfo("FileManager initialized");
    } else {
        // Logging
        loggerError("Couldn't initialize FileManager");
    }

    // Initialize Logger
    loggerInit();

    // Initialize SensorManager
    const bool sensorManagerInitResult = sensorManagerInit();
    if (sensorManagerInitResult) {
        // Logging
        loggerInfo("SensorManager initialized");
    } else {
        // Logging
        loggerError("Couldn't initialize SensorManager");
    }

    while (true) {
        // TESTING ONLY
        //sensorManagerUpdateFuelLevel();
        //sensorManagerUpdateOilPressure();
        //sensorManagerUpdateWaterTemperature();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
