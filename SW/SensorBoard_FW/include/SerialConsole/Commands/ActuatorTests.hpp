#pragma once

// Project includes
#include "KLine/KLine.hpp"
#include "KLine/Tests.hpp"

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
	void handleCommandAnswer(const KLineMessage& message);

	uint8_t getValueTestValue(const ECU_TEST ecuTest, const int passedValue);

	/*
	 *	Command Functions
	 */
	int listTests(int argc, char** p_argv);

	int executeSwitchTest(int argc, char** p_argv);

	int executeValueTest(int argc, char** p_argv);

	/*
	 *	Private Variables
	 */
	KLine* kline_ = nullptr;

	std::vector<esp_console_cmd_t> commands_;
};
