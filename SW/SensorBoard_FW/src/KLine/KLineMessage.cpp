#include "KLine/KLineMessage.hpp"

// C++ includes
#include <format>

// espidf includes
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr auto TAG = "KLineMessage";

constexpr uint8_t ADDR_ECU = 0x10;
constexpr uint8_t ADDR_PCB = 0xF1;

/*
 *   Public Function Implementations
 */
KLineMessage::KLineMessage(const uint8_t sid)
{
	sender_ = ADDR_PCB;
	target_ = ADDR_ECU;
	sid_ = sid;

	calcLength();
	calcChecksum();
}

KLineMessage::KLineMessage(const uint8_t sid, const std::vector<uint8_t>& payload)
{
	sender_ = ADDR_PCB;
	target_ = ADDR_ECU;
	sid_ = sid;
	payloadSize_ = payload.size();
	if (payloadSize_ > 12) {
		payloadSize_ = 12;
	}
	for (uint8_t i = 0; i < payload.size() && i < 12; i++) {
		payload_[i] = payload.at(i);
	}

	calcLength();
	calcChecksum();
}

std::vector<uint8_t> KLineMessage::getRaw()
{
	std::vector<uint8_t> output;

	calcLength();
	output.push_back((length_ << 4) + command_);
	output.push_back(target_);
	output.push_back(sender_);
	output.push_back(sid_);

	for (uint8_t i = 0; i < payloadSize_; i++) {
		output.push_back(payload_[i]);
	}

	calcChecksum();
	output.push_back(checksum_);

	return output;
}

bool KLineMessage::fromRaw(std::vector<uint8_t>& raw)
{
	if (raw.empty()) {
		ESP_LOGW(TAG, "Passed raw message is empty!");
		return false;
	}

	// Check length
	length_ = (raw.at(0) >> 4) & 0b1111;
	if (length_ < 4 || raw.size() <= length_) {
		ESP_LOGW(TAG, "Passed raw message is too short!");
		return false;
	}

	// Extract
	command_ = raw.at(0) & 0b1111;
	target_ = raw.at(1);
	sender_ = raw.at(2);
	sid_ = raw.at(3);

	payloadSize_ = length_ - 4;
	for (uint8_t i = 4; i < length_; i++) {
		payload_[i - 4] = raw.at(i);
	}

	// Test Checksum
	calcChecksum();
	if (checksum_ != raw.at(length_)) {
		ESP_LOGW(TAG, "Checksum mismatch!");
		return false;
	}

	return true;
}

std::string KLineMessage::toString() const
{
	std::string output;

	output += std::format("0x{:02x} ", (length_ << 4) + (command_ & 0b1111));
	output += std::format("0x{:02x} ", target_);
	output += std::format("0x{:02x} ", sender_);
	output += std::format("0x{:02x} ", sid_);
	for (uint8_t i = 0; i < payloadSize_; i++) {
		output += std::format("0x{:02x} ", payload_[i]);
	}
	output += std::format("0x{:02x} ", checksum_);

	return output;
}

bool KLineMessage::operator==(const KLineMessage& rhs) const
{
	bool equal = true;

	equal &= length_ == rhs.length_;
	equal &= command_ == rhs.command_;
	equal &= sender_ == rhs.sender_;
	equal &= target_ == rhs.target_;
	equal &= sid_ == rhs.sid_;
	equal &= payload_ == rhs.payload_;
	equal &= payloadSize_ == rhs.payloadSize_;
	equal &= checksum_ == rhs.checksum_;

	return equal;
}

/*
 *   Private Function Implementations
 */
void KLineMessage::calcLength()
{
	length_ = 0;

	length_++; // Sender
	length_++; // Target
	length_++; // SID
	length_ += payloadSize_;
	length_++; // CS
}

void KLineMessage::calcChecksum()
{
	uint32_t sum = 0;

	sum += (length_ << 4) + (command_ & 0x0F);
	sum += sender_;
	sum += target_;
	sum += sid_;
	for (uint8_t i = 0; i < payloadSize_; i++) {
		sum += payload_[i];
	}

	checksum_ = sum % 256;
}
