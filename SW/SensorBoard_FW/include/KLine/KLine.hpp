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
	enum class Result
	{
		Ok,
		TxFailed, // Could not write the request to the UART
		EchoMismatch, // Echo differs from the request (collision on the bus)
		NoResponse, // ECU did not answer within P2max
		InvalidResponse, // Incomplete response or checksum mismatch
	};

	// Called from the KLine task, so keep it short and don't block
	using ResponseCallback = std::function<void(Result result, const KLineMessage& response)>;

	KLine();

	~KLine();

	// Queues a request. The callback is called once the ECU answered or the request failed
	bool send(const KLineMessage& message, ResponseCallback rxCb);

private:
	struct Request
	{
		KLineMessage message;
		ResponseCallback callback;
	};

	/*
	 *	Private Functions
	 */
	static void klineTask(void* p_param);

	Result process(Request& request, KLineMessage& response);

	bool transmit(const std::vector<uint8_t>& raw);

	bool receiveEcho(const std::vector<uint8_t>& raw);

	Result receiveResponse(KLineMessage& response);

	/*
	 *	Private Variables
	 */
	bool initialized_ = false;

	TaskHandle_t taskHandle_ = nullptr;
	QueueHandle_t requestQueue_ = nullptr; // Holds Request*
};
