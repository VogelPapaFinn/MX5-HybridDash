#pragma once

#include "freertos/FreeRTOS.h"

// Component includes
#include "Can.hpp"

/*
 *	Class to receive CAN Messages
 */
class MessageReceiver
{
public:
	MessageReceiver(Can* p_can, CanFrameGroups::GROUP group);

	MessageReceiver(Can* p_can, std::vector<CanFrameGroups::GROUP> groups);

	~MessageReceiver();

	// Called by the CAN driver when a new frame was received
	void receiveMessage(Can::Frame_t frame);

	void startCanRxTask();

	void stopCanRxTask();

	virtual void handleCanFrame(const Can::Frame_t& frame) = 0;

protected:
	Can* can_ = nullptr;
	std::vector<CanFrameGroups::GROUP> groups_;

	QueueHandle_t receivedFramesQueue_ = nullptr;
	TaskHandle_t handleMessageTask_ = nullptr;

private:
	static void handleMessageTask(void* p_param);
};
