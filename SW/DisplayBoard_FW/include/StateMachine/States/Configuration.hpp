#pragma once

// Project includes
#include "Can.hpp"
#include "CanReceiver.hpp"
#include "Config.hpp"
#include "Filesystem.hpp"
#include "StateMachine/State.hpp"

class Configuration : public State, public CanReceiver
{
public:
	Configuration(Filesystem* p_fs, Config* p_config, Can* p_can);

	void enter() override;

	void quit() override;

private:
	/*
	 *   Private Functions
	 */
	void handleCanFrame(const Can::Frame_t& frame) override;

	void broadcastStateEntering();

	void ackCanId();

	void ackScreen();

	void ackRotation();

	void saveConfiguration();

	/*
	 *   Private Variables
	 */
	enum SUBSTATE
	{
		UNDEFINED = 0,
		SET_CAN_ID,
		SET_SCREEN,
		SET_ROTATION,
		SAVE_CONFIGURATION
	};
	uint8_t substate_ = UNDEFINED;

	Filesystem* fs_ = nullptr;
	Config* config_ = nullptr;

	int canId_ = -1;
	int screen_ = -1;
	int rotation_ = -1;
};
