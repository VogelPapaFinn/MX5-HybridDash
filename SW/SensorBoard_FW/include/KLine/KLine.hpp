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
 *	Class
 */
class KLine
{
public:
	KLine();

	~KLine();

	bool init();

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
	bool setUartGpios();

	//! \note Not working with TJA1021 but not needed for communication!
	bool baud5Init();
	void send33Init();

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
