#pragma once

// Project includes
#include "Can.hpp"
#include "Config.hpp"
#include "StateMachine/State.hpp"

// espidf includes
#include "esp_adc/adc_oneshot.h"

class Startup : public State
{
public:
	Startup(Filesystem* p_fs, Config* p_config, Can* p_can);

	void enter() override;

	void quit() override;

private:
	/*
	 *   Private Functions
	 */
	bool loadConfig();

	bool loadDefaultConfig();

	bool isFirstStartup();

	bool isUpdateAvailable();

	/*
	 *   Private Variables
	 */
	Filesystem* fs_ = nullptr;
	Config* config_ = nullptr;
	Can* can_ = nullptr;
};
