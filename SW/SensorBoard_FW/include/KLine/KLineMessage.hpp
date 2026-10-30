#pragma once

// C++ includes
#include <array>
#include <cstdint>
#include <string>
#include <vector>

class KLineMessage
{
public:
	KLineMessage() = default;

	KLineMessage(const uint8_t command, const uint8_t sender, const uint8_t target, const uint8_t sid,
				 const std::vector<uint8_t>& payload);

	std::vector<uint8_t> getRaw();

	bool fromRaw(std::vector<uint8_t>& raw);

	std::string toString();

	bool operator==(const KLineMessage& rhs) const;

private:
	/*
	 *	Private Functions
	 */
	void calcLength();

	void calcChecksum();

	/*
	 *	Private Variables
	 */
	uint8_t length_ : 4 = 0;
	uint8_t command_ : 4 = 0;
	uint8_t sender_ = 0;
	uint8_t target_ = 0;
	uint8_t sid_ = 0;
	std::array<uint8_t, 10> payload_ = {};
	uint8_t payloadSize_ = 0;
	uint8_t checksum_ = 0;
};
