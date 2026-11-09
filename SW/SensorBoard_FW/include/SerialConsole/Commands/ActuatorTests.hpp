#pragma once

// Project includes
#include "KLine/KLine.hpp"

// C++ includes
#include <vector>

// espidf includes
#include "esp_console.h"

class ActuatorTests
{
public:
	ActuatorTests(KLine* p_kline);

	void registerCommands();

private:
	/*
	 *	Private Functions
	 */

	/*
	 *	Command Functions
	 */
	void handleCommandAnswer(const KLineMessage& message);

	int listTests(int argc, char** p_argv);

	int executeSwitchTest(int argc, char** p_argv);

	/*
	 *	Private Variables
	 */
	KLine* kline_ = nullptr;

	std::vector<esp_console_cmd_t> commands_;
};
