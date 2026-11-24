#pragma once

// espidf includes
#include "freertos/FreeRTOS.h"

class MutexGrabber
{
public:
	MutexGrabber() = delete;
	MutexGrabber(const MutexGrabber&) = delete;
	MutexGrabber& operator=(const MutexGrabber&) = delete;

	MutexGrabber(SemaphoreHandle_t p_mutex) : mutex_(p_mutex) { xSemaphoreTake(p_mutex, portMAX_DELAY); }

	~MutexGrabber() { xSemaphoreGive(mutex_); }

private:
	SemaphoreHandle_t mutex_ = nullptr;
};
