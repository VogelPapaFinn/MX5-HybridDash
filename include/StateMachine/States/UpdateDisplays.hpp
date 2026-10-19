#pragma once

// Project includes
#include "Can.hpp"
#include "Config.hpp"
#include "MessageReceiver.hpp"
#include "StateMachine/State.hpp"
#include "WifiHost.hpp"

/*
 *	Class implementation
 */
class UpdateDisplays : public State, public MessageReceiver
{
public:
	UpdateDisplays(Config* p_config, Can* p_can);

	void enter() override;

	void quit() override;

private:
	/*
	 *   Private Functions
	 */
	void startCanRxTask();
	void stopCanRxTask();

	void enterNextState();
	void handleCanFrame(const Can::Frame_t& frame);

	// STARTING_AP
	void startWifiAP();

	// ENTERING_UPDATE_MODE
	void putDisplaysIntoUpdateMode();

	std::vector<std::vector<char>> getAsPackages(const std::string& src);

	// TRANSMITTING_SSID
	void transmitSsidPackages(const std::vector<std::vector<char>>& ssidPackages);

	// TRANSMITTING_PSSWD
	void transmitPsswdPackages(const std::vector<std::vector<char>>& psswdPackages);

	// TRANSMITTING_HOST_IP
	void transmitHostIp();

	// JOIN_WIFI
	void registerWifiEventHandlers();
	void orderDisplaysToJoinWifi();
	void unregisterWifiEventHandlers();

	// EXECUTE UPDATE
	void orderDisplaysToExecuteUpdate();

	/*
	 *   Private Task Functions
	 */
	static void canRxTask(void* p_param);

	/*
	 *   Private Variables
	 */
	enum SUBSTATE
	{
		UNDEFINED = 0,
		STARTING_AP,
		ENTERING_UPDATE_MODE,
		TRANSMITTING_SSID,
		TRANSMITTING_PSSWD,
		TRANSMITTING_HOST_IP,
		JOIN_WIFI,
		EXECUTING_UPDATE
	};

	uint8_t substate_ = UNDEFINED;

	Config* config_ = nullptr;
	Can* can_ = nullptr;
	WifiHost wifi_;

	TaskHandle_t canRxTask_ = nullptr;

	esp_event_handler_instance_t wifiEventHandler_ = nullptr;
};
