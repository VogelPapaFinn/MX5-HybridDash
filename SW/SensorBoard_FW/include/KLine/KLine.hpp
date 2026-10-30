#pragma once

// C++ includes
#include <cstdint>
#include <functional>
#include <vector>

// Project includes
#include "KLine/KLineMessage.hpp"

// espidf includes
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

/*
 *	constexpr
 */
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

	bool send(const KLineMessage& message, std::function<void(KLineMessage&)> rxCb);

private:
	struct Request
	{
		KLineMessage message;
		std::function<void(KLineMessage&)> callback;
	};

	/*
	 *	Private Functions
	 */
	bool setupUart();

	static void klineTask(void* p_param);

	KLineMessage process(Request& request);

	bool transmit(const std::vector<uint8_t>& raw);

	bool receiveEcho(const std::vector<uint8_t>& raw);

	KLineMessage receiveResponse();

	/*
	 *	Private Variables
	 */
	bool initialized_ = false;

	TaskHandle_t taskHandle_ = nullptr;
	QueueHandle_t requestQueue_ = nullptr;
};
