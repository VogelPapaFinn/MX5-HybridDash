#pragma once

#include "freertos/FreeRTOS.h"

// Component includes
#include "Can.hpp"

/*
 *	Class to receive CAN Messages
 */
class CanReceiver
{
public:
	CanReceiver(Can* p_can);

	~CanReceiver();

	// Called by the CAN driver when a new frame was received
	void receiveMessage(Can::Frame_t frame);

	void startCanRxTask();

	void stopCanRxTask();

	virtual void handleCanFrame(const Can::Frame_t& frame) = 0;

protected:
	Can* can_ = nullptr;

	QueueHandle_t receivedFramesQueue_ = nullptr;
	TaskHandle_t handleMessageTask_ = nullptr;

private:
	static void handleMessageTask(void* p_param);
};
