#pragma once

// Project includes

// C++ includes
#include <format>
#include <functional>
#include <list>
#include <map>
#include <vector>


// espidf includes
#include "KLine/KLineMessage.hpp"
#include "freertos/FreeRTOS.h"


/*
 *	constexpr
 */
constexpr uint16_t INTERNAL_BUFFER_SIZE_RX = 64;

constexpr uint8_t MIN_MESSAGE_LENGTH_B = 5;

constexpr uint8_t SID_RDBI = 0x22; // Read Data By Identifier
constexpr uint8_t SID_READ_FLASH = 0x23; // Unknown SID, but used for reading the ECU ID
constexpr uint8_t SID_RESPONSE = 0x40;

/*
 *	Class
 */
class KLine
{
public:
	KLine();

	~KLine();

	bool send(KLineMessage& message, std::function<void()>* rxCb);

private:
	/*
	 *	Private Functions
	 */
	static void uartTxTask(void* p_param);
	void sendEachByte(KLineMessage& message);

	static void uartRxTask(void* p_param);
	bool isDataAMessage(std::vector<uint8_t>& data);

	static void noAnswerReceived(TimerHandle_t p_timer);

	/*
	 *	Private Variables
	 */
	bool initialized_ = false;

	QueueHandle_t uartQueueHandle_ = nullptr;

	TimerHandle_t waitForAnswerTimeout_ = nullptr;
	TimerHandle_t waitForByteTimeout_ = nullptr;

	TaskHandle_t txTaskHandle_ = nullptr;
	TaskHandle_t rxTaskHandle_ = nullptr;
	std::vector<uint8_t> rxMessageFragments_;

	KLineMessage echo_;

	QueueHandle_t pendingMessagesQueue_ = nullptr;
	std::map<KLineMessage, std::function<void()>*> observers_;
};
