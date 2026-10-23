#pragma once

// Project includes
#include "Can.hpp"
#include "Config.hpp"
#include "Filesystem.hpp"
#include "StateMachine/State.hpp"

class Startup : public State
{
public:
	Startup(Filesystem* p_fs, Config** p_config, Can* p_can);

	void enter() override;

	void quit() override;

private:
	/*
	 *   Public Functions
	 */
	bool loadConfig();

	bool loadDefaultConfig();

	bool isConfigValid();

	void forceCanId();

	/*
	 *   Public Variables
	 */
	Filesystem* fs_ = nullptr;
	Config** configPtr_ = nullptr;
	Can* can_ = nullptr;
};
