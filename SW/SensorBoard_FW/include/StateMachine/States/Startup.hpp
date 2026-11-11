#pragma once

// Project includes
#include "Can.hpp"
#include "Config.hpp"
#include "Driver/Display.hpp"
#include "StateMachine/State.hpp"

// espidf includes
#include "esp_adc/adc_oneshot.h"

class Startup : public State
{
public:
	Startup(Filesystem* p_fs, Config* p_config, Can* p_can, std::vector<Display>* p_displays);

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
	std::vector<Display>* displays_ = nullptr;
};
