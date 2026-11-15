#pragma once

// C++ includes
#include <array>
#include <cstdio>
#include <string>

// espidf includes
#include "esp_log_write.h"
#include "freertos/FreeRTOS.h"

/*
 *   constepxr
 */
constexpr unsigned int LOG_LINE_BUFFER_SIZE = 50; // Strings

/*
 *   Class
 */
class Logger
{
public:
	static bool init(FILE* p_logFile);

private:
	/*
	 *   Private Functions
	 */
	static int logHook(const char* p_fmt, va_list args);

	static void logTask(void* p_param);

	static void flushFile();

	/*
	 *   Private Variables
	 */
	static bool s_initialized;

	static vprintf_like_t s_uartLogHook;
	static FILE* s_logFile;

	static TaskHandle_t s_logTaskHandle;
	static QueueHandle_t s_logQueueHandle;
};
