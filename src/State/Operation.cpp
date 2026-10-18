#include "State/Operation.hpp"

// Project includes
#include "Driver/Display.hpp"
#include "Sensors/HwSensor/FuelLevel.hpp"
#include "Sensors/HwSensor/LeftIndicator.hpp"
#include "Sensors/HwSensor/OilPressure.hpp"
#include "Sensors/HwSensor/RightIndicator.hpp"
#include "Sensors/HwSensor/Rpm.hpp"
#include "Sensors/HwSensor/Speed.hpp"
#include "Sensors/HwSensor/WaterTemperature.hpp"
#include "State/StateEvents.hpp"
#include "Wifi.hpp"
#include "WifiHost.hpp"
#include "WifiJoin.hpp"

// espidf includes
#include "CanGroupsAndFunctions.hpp"
#include "esp_event.h"
#include "esp_log.h"

/*
 *	constexpr
 */
constexpr auto TAG = "Operation";

/*
 *	Public Function implementations
 */
Operation::Operation(Filesystem* p_fs, adc_oneshot_unit_handle_t p_adc1, KLine* p_kline, Wifi* p_wifi, Config* p_config,
					 Can* p_can, std::vector<Display>* p_displays)
{
	fs_ = p_fs;
	adc1_ = p_adc1;
	kline_ = p_kline;
	wifi_ = p_wifi;
	config_ = p_config;
	can_ = p_can;
	displays_ = p_displays;
}

void Operation::enter() {}

void Operation::quit() {}

void Operation::handleCanFrame(const Can::Frame_t* p_frame) const
{
	if (p_frame->group != CanFrameGroups::GROUP::WIFI) {
		return;
	}

	/*
	 * Act depending on the function
	 */
	switch (p_frame->function) {
		/*
		 *	Display connected to the Wifi
		 */
		case CanFrameGroups::WIFI::JOIN_WIFI:
			{
				if (!p_frame->answer) {
					return;
				}

				static uint8_t s_counter = 0;
				esp_rom_printf("Display %d joined Wifi\n", ++s_counter);
			}
			break;

		/*
		 *	Display executed uupdate
		 */
		case CanFrameGroups::WIFI::EXECUTE_UPDATE:
			{
				if (!p_frame->answer) {
					return;
				}

				static uint8_t s_counter = 0;
				ESP_LOGI(TAG, "Display %d executed update successfully!", p_frame->sender);

				/*
				 * Restart all displays & ourselves when they are ready
				 */
				if (++s_counter >= 3) {
					// Create basic CAN frame
					Can::Frame_t txFrame;
					txFrame.sender = CAN_MASTER_ID;
					txFrame.target = CAN_BROADCAST_ID;
					txFrame.group = CanFrameGroups::GROUP::CONFIGURATION;
					txFrame.function = CanFrameGroups::CONFIGURATION::RESTART;

					// Send the frame
					can_->queueFrame(txFrame);

					// Restart after 1 seconds
					vTaskDelay(pdMS_TO_TICKS(1000));
					esp_restart();
				}

				/*
				 *	Execute the update on the next display
				 */
				executeDisplayUpdate(this->displays_->at(s_counter).getCanId());
			}
			break;

		default:
			break;
	}
}

/*
 *	Private Function Implementations
 */
void Operation::setupWifi()
{
	Wifi::WIFI_TYPE wifiType = Wifi::WIFI_TYPE::HOST;
	const auto json = config_->getWriteAccess();

	/*
	 *	Delete old Wifi
	 */
	if (wifi_ != nullptr) {
		free(wifi_);
		wifi_ = nullptr;
	}

	/*
	 *	Get Wifi mode
	 */
	if ((*json)["ForceWifiMode"].is<unsigned int>()) {
		wifiType = static_cast<Wifi::WIFI_TYPE>((*json)["ForceWifiMode"].as<int>());
	}

	/*
	 *	Setup Wifi Host
	 */
	if (wifiType == Wifi::WIFI_TYPE::HOST) {
		// Create Wifi
		wifi_ = new WifiHost();

		// Initialize Wifi
		wifi_->setSsid((*json)["WifiHost"]["ssid"]);
		wifi_->setPassword((*json)["WifiHost"]["password"]);
	}

	/*
	 *	Setup Wifi Join
	 */
	else if (wifiType == Wifi::WIFI_TYPE::JOIN) {
		// Create Wifi
		wifi_ = new WifiJoin();

		// Initialize Wifi
		wifi_->setSsid((*json)["WifiJoin"]["ssid"]);
		wifi_->setPassword((*json)["WifiJoin"]["password"]);
	}

	// Error
	else {
		ESP_LOGW(TAG, "Failed to initialize wifi. Continuing without it");
		return;
	}

	/*
	 *	Start Wifi
	 */
	wifi_->callOnSuccess(
		[this]
		{
			// Initialize display Wifi
			this->connectDisplaysToWifi();
		});

	wifi_->start();
}


void Operation::connectDisplaysToWifi() const
{
	ESP_LOGI(TAG, "Starting to transmit SSID and Password to the displays");

	/*
	 *	Transmit own IP
	 */
	// Build the basic CAN frame
	Can::Frame_t transmitMasterIpFrame;
	transmitMasterIpFrame.sender = CAN_MASTER_ID;
	transmitMasterIpFrame.target = CAN_BROADCAST_ID;
	transmitMasterIpFrame.group = CanFrameGroups::GROUP::WIFI;
	transmitMasterIpFrame.function = CanFrameGroups::WIFI::SET_MASTER_IP;
	transmitMasterIpFrame.dataLengthCode = 4;

	// Add the master IP address
	const auto& ip = wifi_->getIp();
	transmitMasterIpFrame.data[0] = ip[0];
	transmitMasterIpFrame.data[1] = ip[1];
	transmitMasterIpFrame.data[2] = ip[2];
	transmitMasterIpFrame.data[3] = ip[3];

	// Send it
	can_->queueFrame(transmitMasterIpFrame);

	/*
	 *	Split the SSID into transferable packages
	 */
	const auto& ssid = wifi_->getSsid();
	std::vector<std::vector<char>> allSsidPackages;
	std::vector<char> ssidPackage;

	// Split the ssid up into packages with a size of max 8 bytes/chars
	for (const auto& c : ssid) {
		if (ssidPackage.size() >= 8) {
			allSsidPackages.push_back(ssidPackage);
			ssidPackage.clear();
		}

		ssidPackage.push_back(c);
	}

	// Add the last package too
	allSsidPackages.push_back(ssidPackage);

	/*
	 * Transmit the SSID
	 */
	for (const auto& package : allSsidPackages) {
		// Build the basic CAN frame
		Can::Frame_t ssidPackageFrame;
		ssidPackageFrame.sender = CAN_MASTER_ID;
		ssidPackageFrame.target = CAN_BROADCAST_ID;
		ssidPackageFrame.group = CanFrameGroups::GROUP::WIFI;
		ssidPackageFrame.function = CanFrameGroups::WIFI::SET_SSID;
		ssidPackageFrame.dataLengthCode = package.size();

		// Fill in the SSID package
		std::copy(package.begin(), package.end(), ssidPackageFrame.data);

		// Send the frame
		can_->queueFrame(ssidPackageFrame);
	}

	/*
	 *	Split the Password into transferable packages
	 */
	const auto& password = wifi_->getPassword();
	std::vector<std::vector<char>> allPsswdPackages;
	std::vector<char> psswdPackage;

	// Split the password up into packages with a size of max 8 bytes/chars
	for (const auto& c : password) {
		if (psswdPackage.size() >= 8) {
			allPsswdPackages.push_back(psswdPackage);
			psswdPackage.clear();
		}

		psswdPackage.push_back(c);
	}

	// Add the last package too
	allPsswdPackages.push_back(psswdPackage);

	/*
	 * Transmit the password
	 */
	for (const auto& package : allPsswdPackages) {
		// Build the basic CAN frame
		Can::Frame_t passwordPackageFrame;
		passwordPackageFrame.sender = CAN_MASTER_ID;
		passwordPackageFrame.target = CAN_BROADCAST_ID;
		passwordPackageFrame.group = CanFrameGroups::GROUP::WIFI;
		passwordPackageFrame.function = CanFrameGroups::WIFI::SET_PASSWORD;
		passwordPackageFrame.dataLengthCode = package.size();

		// Fill in the password package
		std::copy(package.begin(), package.end(), passwordPackageFrame.data);

		// Send the frame
		can_->queueFrame(passwordPackageFrame);
	}

	/*
	 *	Instruct the displays to join the Wifi
	 */
	Can::Frame_t joinWifiFrame;
	joinWifiFrame.sender = CAN_MASTER_ID;
	joinWifiFrame.target = CAN_BROADCAST_ID;
	joinWifiFrame.group = CanFrameGroups::GROUP::WIFI;
	joinWifiFrame.function = CanFrameGroups::WIFI::JOIN_WIFI;
	joinWifiFrame.dataLengthCode = 0;

	can_->queueFrame(joinWifiFrame);
}

void Operation::executeDisplayUpdate(const uint8_t displayId) const
{
	ESP_LOGI(TAG, "Executing update for display with ID %d!", displayId);

	// Build the basic CAN frame
	Can::Frame_t frame;
	frame.sender = CAN_MASTER_ID;
	frame.target = displayId;
	frame.group = CanFrameGroups::GROUP::WIFI;
	frame.function = CanFrameGroups::WIFI::EXECUTE_UPDATE;

	// Send the frame
	can_->queueFrame(frame);
}
