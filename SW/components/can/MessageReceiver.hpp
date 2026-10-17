#include "freertos/FreeRTOS.h"

// Component includes
#include "Can.hpp"

/*
 *	Class to receive CAN Messages
 */
class MessageReceiver
{
public:
	MessageReceiver()
	{
		// Create Queue
		receivedFramesQueue_ = xQueueCreate(10, sizeof(Can::Frame_t));
	}

	void receiveMessage(Can::Frame_t frame)
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xQueueSendFromISR(receivedFramesQueue_, &frame, &xHigherPriorityTaskWoken);
	}

protected:
	QueueHandle_t receivedFramesQueue_ = nullptr;
};
