#pragma once

// C includes
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 *	Static Variables
 */
//! \brief Defines what should be logged:
//! 0 - nothing
//! 1 - only critical errors
//! 2 - critical errors & errors
//! 3 - warnings & critical errors & errors
//! 4 - everything
//! 5 - everything + Debugging
extern const int LOGGING_LEVEL;

/*
 *  Functions
 */
//! \brief Initializes the logger
void loggerInit(void);

//! \brief Logs a message with level 'Debug'
//! \param message The message that should be logged
//! \param ... Additional parameters
void loggerDebug(const char* message, ...);

//! \brief Logs a message with level 'Info'
//! \param message The message that should be logged
//! \param ... Additional parameters
void loggerInfo(const char* message, ...);

//! \brief Logs a message with level 'Warn'
//! \param message The message that should be logged
//! \param ... Additional parameters
void loggerWarn(const char* message, ...);

//! \brief Logs a message with level 'Error'. 'Error' means that something failed
//! that should have succeeded but the system can continue operating.
//! \param message The message that should be logged
//! \param ... Additional parameters
void loggerError(const char* message, ...);

//! \brief Logs a message with level 'Critical'. 'Critical' means that something failed
//! that should have succeeded and the system is not able to keep operating.
//! \param message The message that should be logged
//! \param ... Additional parameters
void loggerCritical(const char* message, ...);
