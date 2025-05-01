/* Includes */

// Project
#include "Core/Core.h"
#include "FileManager/FileManager.h"
#include "GUI/GUI.h"
#include "Logger/Logger.h"
#include "SensorManager/SensorManager.h"

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

    // Initialize the GUI system
    if (guiInit()) {
        // Logging
        loggerInfo("GUI system initialized");
    } else {
        // Logging
        loggerCritical("Couldn't initialize GUI system");

        // Return, as there is no use in continuing
        return;
    }

    // Initialize the Core
    coreInit();

    while (true) {
        // TESTING ONLY
        //sensorManagerUpdateFuelLevel();
        //sensorManagerUpdateOilPressure();
        //sensorManagerUpdateWaterTemperature();
        //sensorManagerUpdateSpeed();
        //sensorManagerUpdateRPM();
        //sensorManagerUpdateInternalTemperature();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
