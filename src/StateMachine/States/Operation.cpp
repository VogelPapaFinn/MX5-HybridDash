#include "StateMachine/States/Operation.hpp"

// espidf includes
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "Operation";

/*
 *	Public Function implementations
 */
Operation::Operation(SensorHandler* p_sensorHandler) { sensorHandler_ = p_sensorHandler; }

void Operation::enter()
{
	ESP_LOGI(TAG, "Entering Operation state");
	sensorHandler_->start();
}

void Operation::quit()
{
	ESP_LOGI(TAG, "Quitting Operation state");
	sensorHandler_->stop();
}

//
//
// void Operation::connectDisplaysToWifi() const
//{
//	ESP_LOGI(TAG, "Starting to transmit SSID and Password to the displays");
//
//	/*
//	 *	Transmit own IP
//	 */
//	// Build the basic CAN frame
//	Can::Frame_t transmitMasterIpFrame;
//	transmitMasterIpFrame.sender = CAN_MASTER_ID;
//	transmitMasterIpFrame.target = CAN_BROADCAST_ID;
//	transmitMasterIpFrame.group = CanFrameGroups::GROUP::WIFI;
//	transmitMasterIpFrame.function = CanFrameGroups::WIFI::SET_MASTER_IP;
//	transmitMasterIpFrame.dataLengthCode = 4;
//
//	// Add the master IP address
//	const auto& ip = wifi_->getIp();
//	transmitMasterIpFrame.data[0] = ip[0];
//	transmitMasterIpFrame.data[1] = ip[1];
//	transmitMasterIpFrame.data[2] = ip[2];
//	transmitMasterIpFrame.data[3] = ip[3];
//
//	// Send it
//	can_->queueFrame(transmitMasterIpFrame);
//
//
//	/*
//	 * Transmit the SSID
//	 */
//	for (const auto& package : allSsidPackages) {
//		// Build the basic CAN frame
//		Can::Frame_t ssidPackageFrame;
//		ssidPackageFrame.sender = CAN_MASTER_ID;
//		ssidPackageFrame.target = CAN_BROADCAST_ID;
//		ssidPackageFrame.group = CanFrameGroups::GROUP::WIFI;
//		ssidPackageFrame.function = CanFrameGroups::WIFI::SET_SSID;
//		ssidPackageFrame.dataLengthCode = package.size();
//
//		// Fill in the SSID package
//		std::copy(package.begin(), package.end(), ssidPackageFrame.data);
//
//		// Send the frame
//		can_->queueFrame(ssidPackageFrame);
//	}
//
//	/*
//	 *	Split the Password into transferable packages
//	 */
//	const auto& password = wifi_->getPassword();
//	std::vector<std::vector<char>> allPsswdPackages;
//	std::vector<char> psswdPackage;
//
//	// Split the password up into packages with a size of max 8 bytes/chars
//	for (const auto& c : password) {
//		if (psswdPackage.size() >= 8) {
//			allPsswdPackages.push_back(psswdPackage);
//			psswdPackage.clear();
//		}
//
//		psswdPackage.push_back(c);
//	}
//
//	// Add the last package too
//	allPsswdPackages.push_back(psswdPackage);
//
//	/*
//	 * Transmit the password
//	 */
//	for (const auto& package : allPsswdPackages) {
//		// Build the basic CAN frame
//		Can::Frame_t passwordPackageFrame;
//		passwordPackageFrame.sender = CAN_MASTER_ID;
//		passwordPackageFrame.target = CAN_BROADCAST_ID;
//		passwordPackageFrame.group = CanFrameGroups::GROUP::WIFI;
//		passwordPackageFrame.function = CanFrameGroups::WIFI::SET_PASSWORD;
//		passwordPackageFrame.dataLengthCode = package.size();
//
//		// Fill in the password package
//		std::copy(package.begin(), package.end(), passwordPackageFrame.data);
//
//		// Send the frame
//		can_->queueFrame(passwordPackageFrame);
//	}
//
//	/*
//	 *	Instruct the displays to join the Wifi
//	 */
//	Can::Frame_t joinWifiFrame;
//	joinWifiFrame.sender = CAN_MASTER_ID;
//	joinWifiFrame.target = CAN_BROADCAST_ID;
//	joinWifiFrame.group = CanFrameGroups::GROUP::WIFI;
//	joinWifiFrame.function = CanFrameGroups::WIFI::JOIN_WIFI;
//	joinWifiFrame.dataLengthCode = 0;
//
//	can_->queueFrame(joinWifiFrame);
// }
//
// void Operation::executeDisplayUpdate(const uint8_t displayId) const
//{
//	ESP_LOGI(TAG, "Executing update for display with ID %d!", displayId);
//
//	// Build the basic CAN frame
//	Can::Frame_t frame;
//	frame.sender = CAN_MASTER_ID;
//	frame.target = displayId;
//	frame.group = CanFrameGroups::GROUP::WIFI;
//	frame.function = CanFrameGroups::WIFI::EXECUTE_UPDATE;
//
//	// Send the frame
//	can_->queueFrame(frame);
// }
