/* --- Includes --- */
#include "Logger/Logger.h"

/* --- Private Defines & Macros --- */

// The log file on the SD Card
FILE *logFileSDCard_ = NULL;
// The log file on the spiffs partition
FILE *logFileSpiffs_ = NULL;

/* --- Private Variables, Typedefs etc. --- */

/* --- Function implementations --- */
void loggerInit(void) {
    // Should we create a file on the internal spiffs partition?
    if (LOGGER_SAVE_INTERNAL) {
        // Ask FileManager to create a file for us
        if (fileManagerCreateFile(LOGGER_FILE_NAME, LOCATION_INTERNAL)) {
            // a+ -> Open for appending & Create file if it not exist
            logFileSpiffs_ = fileManagerOpenFile(LOGGER_FILE_NAME, "a+", LOCATION_INTERNAL);
        }

        // Was it successful?
        if (logFileSpiffs_) {
            // Logging
            loggerInfo("Successfully created log file on the internal partition");
        } else {
            // Logging
            loggerError("Failed to create log file on the internal partition");
        }
    }

    // Should we create a file on the SD Card?
    if (LOGGER_SAVE_ON_SDCARD) {
        // Ask FileManager to create a file for us
        if (fileManagerCreateFile(LOGGER_FILE_NAME, LOCATION_SDCARD)) {
            // a+ -> Open for appending & Create file if it not exist
            logFileSDCard_ = fileManagerOpenFile(LOGGER_FILE_NAME, "a+", LOCATION_SDCARD);
        }

        // Was it successful?
        if (logFileSDCard_) {
            // Logging
            loggerInfo("Successfully created log file on the SD Card");
        } else {
            // Logging
            loggerError("Failed to create log file on the SD Card");
        }
    }
}

void loggerLog(const char *level, const char *message, const va_list args) {
    // Build the whole log message
    char *fullMessage = malloc(strlen(level) + strlen(" ") + strlen(message) + strlen("\n") + 1);
    sprintf(fullMessage, "%s%s%s%s", level, " ", message, "\n");

    // Should we log to an internal file?
    if (LOGGER_SAVE_INTERNAL && logFileSpiffs_) {
        vfprintf(logFileSpiffs_, fullMessage, args);
    }
    // Should we log to a file on the SD Card?
    if (LOGGER_SAVE_ON_SDCARD && logFileSDCard_) {
        vfprintf(logFileSDCard_, fullMessage, args);
    }
    // Should we log to the USB port?
    if (LOGGER_SEND_TO_USB) {
        // Print the message
        vprintf(fullMessage, args);
    }

    // Free the full message
    free(fullMessage);
}

void loggerInfo(const char *message, ...) {
    // Check log level
    if (LOGGING_LEVEL < 4) return;

    // Get all arguments
    va_list args;
    va_start(args, message);

    // Log with INFO level
    loggerLog("[INFO]", message, args);

    // Free arguments
    va_end(args);
}

void loggerWarn(const char *message, ...) {
    // Check log level
    if (LOGGING_LEVEL < 3) return;

    // Get all arguments
    va_list args;
    va_start(args, message);

    // Log with WARNING level
    loggerLog("[WARNING]", message, args);

    // Free arguments
    va_end(args);
}

void loggerError(const char *message, ...) {
    // Check log level
    if (LOGGING_LEVEL < 2) return;

    // Get all arguments
    va_list args;
    va_start(args, message);

    // Log with ERROR level
    loggerLog("[ERROR]", message, args);

    // Free arguments
    va_end(args);
}

void loggerCritical(const char *message, ...) {
    // Check log level
    if (LOGGING_LEVEL < 1) return;

    // Get all arguments
    va_list args;
    va_start(args, message);

    // Log with CRITICAL level
    loggerLog("[CRITICAL]", message, args);

    // Free arguments
    va_end(args);
}