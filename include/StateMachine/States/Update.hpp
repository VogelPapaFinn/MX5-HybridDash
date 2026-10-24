#pragma once

// Project includes
#include "Can.hpp"
#include "MessageReceiver.hpp"
#include "StateMachine/State.hpp"
#include "WifiJoin.hpp"

class Update : public State, public MessageReceiver
{
public:
	Update(Can* p_can);

	void enter() override;

	void quit() override;

private:
	/*
	 *   Private Functions
	 */
	void handleCanFrame(const Can::Frame_t& frame) override;

	void broadcastEnteredUpdateMode();

	void joinWifi();

	void executeUpdate();

	void reportUpdateSuccess();

	/*
	 *   Private Variables
	 */
	enum SUBSTATE
	{
		UNDEFINED = 0,
		RECEIVING_SSID,
		RECEIVING_PSSWD,
		RECEIVING_HOST_IP,
		JOIN_WIFI,
		EXECUTING_UPDATE
	};
	uint8_t substate_ = UNDEFINED;

	WifiJoin wifi_;
	std::string ssid_;
	std::string psswd_;
	std::array<uint8_t, 4> masterIp_;
};
