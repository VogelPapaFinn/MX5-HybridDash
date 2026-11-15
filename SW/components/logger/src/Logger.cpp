#include "../Logger.hpp"

// C++ includes
#include <format>

// espidf includes
#include "esp_log.h"

/*
 *   constepxr
 */
constexpr unsigned int LOG_SAVE_INTERVAL_MS = 60000;

/*
 *   Static Variables
 */
bool Logger::s_initialized = false;

vprintf_like_t Logger::s_uartLogHook = nullptr;
FILE* Logger::s_logFile = nullptr;

TaskHandle_t Logger::s_logTaskHandle = nullptr;
QueueHandle_t Logger::s_logQueueHandle = nullptr;

/*
 *   Public Function Implementations
 */
bool Logger::init(FILE* p_logFile)
{
	if (p_logFile == nullptr || s_initialized) {
		return false;
	}

	s_logFile = p_logFile;

	s_logQueueHandle = xQueueCreate(100, sizeof(std::string*));
	if (s_logQueueHandle == nullptr) {
		return false;
	}

	if (xTaskCreate(logTask, "LogTask", 4096, nullptr, 2, &s_logTaskHandle) != pdPASS) {
		return false;
	}

	s_uartLogHook = esp_log_set_vprintf(&logHook);

	s_initialized = true;
	return true;
}

/*
 *   Private Function Implementations
 */

int Logger::logHook(const char* p_fmt, va_list args)
{
	const auto callerTask = xTaskGetCurrentTaskHandle();
	if (callerTask == s_logTaskHandle) {
		return s_uartLogHook(p_fmt, args);
	}

	// Get log line length
	va_list copy;
	va_copy(copy, args);
	const auto logLineLength = vsnprintf(nullptr, 0, p_fmt, copy);
	va_end(copy);

	if (logLineLength <= 0) {
		return s_uartLogHook(p_fmt, args);
	}

	// Convert to std::string
	auto logLine = new std::string(logLineLength, ' ');
	va_copy(copy, args);
	vsnprintf((*logLine).data(), logLineLength + 1, p_fmt, copy); // +1, otherwise /0 is cutoff
	va_end(copy);

	// Send to queue
	if (xQueueSend(s_logQueueHandle, &logLine, 0) == pdFALSE) {
		delete (logLine);
	}

	return s_uartLogHook(p_fmt, args);
}

void Logger::logTask(void* p_param)
{
	TickType_t lastSaveOnTick = 0;

	std::string* logLine = nullptr;
	while (true) {
		if (xQueueReceive(s_logQueueHandle, &logLine, pdMS_TO_TICKS(LOG_SAVE_INTERVAL_MS)) != pdTRUE) {
			flushFile();
			lastSaveOnTick = xTaskGetTickCount();
			continue;
		}

		fprintf(s_logFile, "%s", (*logLine).c_str());
		delete (logLine);

		if (pdTICKS_TO_MS(xTaskGetTickCount() - lastSaveOnTick) >= LOG_SAVE_INTERVAL_MS) {
			flushFile();
			lastSaveOnTick = xTaskGetTickCount();
		}
	}
}

void Logger::flushFile()
{
	fflush(s_logFile);
	fsync(fileno(s_logFile));
}
