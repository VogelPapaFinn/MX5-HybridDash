#pragma once

// Project includes
#include "KLine/KLine.hpp"
#include "KLine/Tests.hpp"

// C++ includes
#include <vector>

// espidf includes
#include "esp_console.h"

class FaultCodes
{
public:
	FaultCodes(KLine* p_kline);

	void registerCommands();

private:
	/*
	 *	Private Functions
	 */
	void requestAmountOfFaultCodes(std::function<void(unsigned int)> cb);

	/*
	 *	Command Functions
	 */
	int checkAmountOfFaultCodes(int argc, char** p_argv);

	int readFaultCodes(int argc, char** p_argv);

	/*
	 *	Private Variables
	 */
	KLine* kline_ = nullptr;

	std::vector<esp_console_cmd_t> commands_;
};
