#pragma once

// Project includes
#include "EcuSensors.hpp"

// C++ includes
#include <list>

// espidf includes
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
	/*
	 *	Public struct
	 */
	struct KLineMessage
	{
		std::vector<uint8_t> rawMessage = {};

		uint8_t length = 0;

		uint8_t command = 0;

		uint8_t sender = 0;

		uint8_t receiver = 0;

		uint8_t sid = 0;

		std::vector<uint8_t> pid = {};

		std::vector<uint8_t> data = {};

		uint8_t checksum = 0;

		void calculateChecksum()
		{
			uint32_t tmp = 0;

			tmp += (length << 4) + (command & 0x0F);
			tmp += sender;
			tmp += receiver;
			tmp += sid;
			for (const auto& p : pid) {
				tmp += p;
			}
			for (const auto& d : data) {
				tmp += d;
			}

			checksum = tmp % 256;
		}

		void fromRawMessage()
		{
			uint8_t i = 0;

			// Get the length & the command of the message
			length = (rawMessage[i] >> 4) & 0x0F;
			command = rawMessage[i++] & 0x0F;

			// Check if the length fits the minimum message size
			if (length < MIN_MESSAGE_LENGTH_B) {
				return;
			}

			// Acquire the sender, received and the SID
			sender = rawMessage[i++];
			receiver = rawMessage[i++];
			sid = rawMessage[i++];

			// Get the PID
			if (sid == SID_READ_FLASH) {
				pid.push_back(rawMessage[i++]);
				pid.push_back(rawMessage[i++]);
				pid.push_back(rawMessage[i++]);
			} else {
				pid.push_back(rawMessage[i++]);
				pid.push_back(rawMessage[i++]);
			}

			// Get all the message data
			for (; i < length; i++) {
				data.push_back(rawMessage[i]);
			}

			// Get the checksum
			checksum = rawMessage[i++];
		}

		void toRawMessage()
		{
			// Set the length & the command of the message
			rawMessage.push_back((length << 4) + command);

			// Set the sender, received and the SID
			rawMessage.push_back(sender);
			rawMessage.push_back(receiver);
			rawMessage.push_back(sid);

			// Set the PID depending on the SID
			for (const auto& p : pid) {
				rawMessage.push_back(p);
			}

			// Set all the message data
			for (const auto& d : data) {
				rawMessage.push_back(d);
			}

			// Set the checksum
			calculateChecksum();
			rawMessage.push_back(checksum);
		}

		std::string toString()
		{
			std::string output;

			output += std::format("0x{:02x} ", (length << 4) + command);
			output += std::format("0x{:02x} ", sender);
			output += std::format("0x{:02x} ", receiver);
			output += std::format("0x{:02x} ", sid);
			for (const auto& p : pid) {
				output += std::format("0x{:02x} ", p);
			}
			for (const auto& d : data) {
				output += std::format("0x{:02x} ", d);
			}
			output += std::format("0x{:02x} ", checksum);

			return output;
		}

		bool operator==(const KLineMessage& rhs) const
		{
			bool equal = true;

			equal &= length == rhs.length;
			equal &= command == rhs.command;
			equal &= sender == rhs.sender;
			equal &= receiver == rhs.receiver;
			equal &= sid == rhs.sid;

			equal &= pid == rhs.pid;
			equal &= data == rhs.data;

			equal &= checksum == rhs.checksum;

			return equal;
		}
	};

	/*
	 *	Public functions
	 */
	KLine();

	~KLine();

	void readEcuId();

	void readPid(uint16_t pid);

	QueueHandle_t getQueue() const;

	SemaphoreHandle_t getLastMessageMutex() const;

	std::list<KLineMessage>& getLastMessagesSent();

private:
	/*
	 *	Private Functions
	 */
	void send(KLineMessage& message);

	/*
	 *	Private Variables
	 */
	bool initialized_ = false;

	QueueHandle_t uartQueueHandle_ = nullptr;

	TaskHandle_t rxTaskHandle_ = nullptr;

	std::list<KLineMessage> lastMessagesSent_;
	SemaphoreHandle_t lastMessageMutex_ = nullptr;
};
