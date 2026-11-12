#pragma once

// Project includes
#include "KLine/KLine.hpp"
#include "SerialConsole/Commands/ActuatorTests.hpp"
#include "SerialConsole/Commands/FaultCodes.hpp"

// espidf includes
#include "esp_console.h"

class SerialConsole
{
public:
	SerialConsole(KLine* p_kline);

	void start();

	void stop();

private:
	/*
	 *   Private Functions
	 */
	void prepareRepl();

	void registerCommands();

	/*
	 *   Private Variables
	 */
	KLine* kline_ = nullptr;

	ActuatorTests actuatorTest_;
	FaultCodes faultCodes_;

	bool useable_ = false;

	esp_console_repl_t* replHandle_ = nullptr;
	esp_console_repl_config_t replConfig_ = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
	const esp_console_dev_usb_serial_jtag_config_t jtagConfig_ = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
};
