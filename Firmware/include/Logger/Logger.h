#ifndef FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_LOGGER
#define FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_LOGGER

/* --- Includes --- */
// C includes
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

// Project includes
#include "FileManager/FileManager.h"

/* --- Defines & Macros --- */
#define LOGGER_FILE_NAME "log.txt"

/* --- Variables, Typedefs etc. --- */
//! \brief Defines if the log file should be saved on the internal data partition
static const bool LOGGER_SAVE_INTERNAL = true;
//! \brief Defines if the log file should be saved on the SDCard
static const bool LOGGER_SAVE_ON_SDCARD = true;
//! \brief Defines if every message should be sent to the USB-C port
static const bool LOGGER_SEND_TO_USB = true;
//! \brief Defines what should be logged:
//! 0 - nothing
//! 1 - only critical errors
//! 2 - critical errors & errors
//! 3 - warnings & critical errors & errors
//! 4 - everything
static const int LOGGING_LEVEL = 4;

/* --- Imported Variables, Typedefs etc. --- */

/* --- Global variables and function (headers) --- */
//! \brief Initializes the logger
void loggerInit(void);

//! \brief Logs a message with level 'Info'
//! \param message The message that should be logged
//! \param ... Additional parameters
void loggerInfo(const char *message, ...);

//! \brief Logs a message with level 'Warn'
//! \param message The message that should be logged
//! \param ... Additional parameters
void loggerWarn(const char *message, ...);

//! \brief Logs a message with level 'Error'. 'Error' means that something failed
//! that should have succeeded but the system can continue operating.
//! \param message The message that should be logged
//! \param ... Additional parameters
void loggerError(const char *message, ...);

//! \brief Logs a message with level 'Critical'. 'Critical' means that something failed
//! that should have succeeded and the system is not able to keep operating.
//! \param message The message that should be logged
//! \param ... Additional parameters
void loggerCritical(const char *message, ...);

#endif// FIRMWARE_INCLUDE_C_HEADER_TEMPLATE_H_LOGGER
