// Project includes
#include "../logger.h"

/*
 *	Variable declaration
 */
const int LOGGING_LEVEL = 5;

/*
 *	Private functions
 */
void loggerLog(const char* level, const char* message, const va_list args)
{
  // Build the whole log message
  char* fullMessage = malloc(strlen(level) + strlen(" ") + strlen(message) + strlen("\n") + 1);
  sprintf(fullMessage, "%s%s%s%s", level, " ", message, "\n");

  // Print the message
  vprintf(fullMessage, args);

  // Free the full message
  free(fullMessage);
}

void loggerDebug(const char* message, ...)
{
  // Check log level
  if (LOGGING_LEVEL < 5) return;

  // Get all arguments
  va_list args;
  va_start(args, message);

  // Log with INFO level
  loggerLog("[DEBUG]", message, args);

  // Free arguments
  va_end(args);
}

void loggerInfo(const char* message, ...)
{
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

void loggerWarn(const char* message, ...)
{
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

void loggerError(const char* message, ...)
{
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

void loggerCritical(const char* message, ...)
{
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
