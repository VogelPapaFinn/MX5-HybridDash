/* Includes */

// Project
#include "FileManager/FileManager.h"
#include "Logger/Logger.h"

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

    fileManager_test();

    while (true) {}
}
