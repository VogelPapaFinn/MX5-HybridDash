#pragma once

// Project includes
#include "Can.hpp"
#include "CanReceiver.hpp"
#include "Config.hpp"
#include "Driver/Display.hpp"
#include "StateMachine/State.hpp"
#include "StateMachine/Utils/TimeoutTimer.hpp"
#include "WifiHost.hpp"

// espidf includes
#include "esp_http_server.h"

/*
 *	Class implementation
 */
class UpdateDisplays : public State, public CanReceiver
{
public:
	UpdateDisplays(Filesystem* p_fs, Config* p_config, Can* p_can, std::vector<Display>* p_displays);

	void enter() override;

	void quit() override;

private:
	/*
	 *   Private Functions
	 */
	void enterNextState();
	void handleCanFrame(const Can::Frame_t& frame) override;

	void restartDisplays();

	// STARTING_AP
	void startWifiAP();

	// STARTIN_WEBSERVER
	bool startWebserver();

	// ENTERING_UPDATE_MODE
	void enterUpdateMode();
	void enteredUpdateMode();

	std::vector<std::vector<char>> getAsPackages(const std::string& src);

	// TRANSMITTING_SSID
	void transmitSsidPackages();

	// TRANSMITTING_PSSWD
	void transmitPsswdPackages();

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
		STARTING_WEBSERVER,
		ENTERING_UPDATE_MODE,
		TRANSMITTING_SSID,
		TRANSMITTING_PSSWD,
		TRANSMITTING_HOST_IP,
		JOIN_WIFI,
		EXECUTING_UPDATE
	};
	uint8_t substate_ = UNDEFINED;

	Filesystem* fs_ = nullptr;
	Config* config_ = nullptr;
	std::vector<Display>* displays_ = nullptr;
	WifiHost wifi_;

	std::vector<std::vector<char>> ssidPackages_;
	std::vector<std::vector<char>> psswdPackages_;

	TaskHandle_t enterUpdateModeTask_ = nullptr;

	httpd_handle_t webserver_ = nullptr;
	const httpd_config_t webserverConfig_ = HTTPD_DEFAULT_CONFIG();
	httpd_uri_t updateUri_ = {};

	TimeoutTimer* timeoutTimer_ = nullptr;

	esp_event_handler_instance_t wifiEventHandler_ = nullptr;

	TaskHandle_t putDisplaysIntoUpdateModeTask_ = nullptr;
};
