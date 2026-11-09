#include "SerialConsole/SerialConsole.hpp"

// espidf includes
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr auto TAG = "SerialConsole";

constexpr size_t MAX_CMDLINE_LENGTH = 256;

/*
 *   Public Function Implementations
 */
SerialConsole::SerialConsole(KLine* p_kline) : actuatorTest_(p_kline)
{
	kline_ = p_kline;

	prepareRepl();
}

void SerialConsole::start() { esp_console_start_repl(replHandle_); }

void SerialConsole::stop() { esp_console_stop_repl(replHandle_); }

/*
 *   Private Function Implementations
 */
void SerialConsole::prepareRepl()
{
	ESP_LOGD(TAG, "Preparing Serial Console");

	replConfig_.prompt = ">> ";
	replConfig_.max_cmdline_length = MAX_CMDLINE_LENGTH;

	if (esp_console_register_help_command() != ESP_OK) {
		ESP_LOGW(TAG, "Failed to register help command. It wont be available in the Serial Console!");
	}

	registerCommands();

	if (esp_console_new_repl_usb_serial_jtag(&jtagConfig_, &replConfig_, &replHandle_) != ESP_OK) {
		ESP_LOGE(
			TAG,
			"Failed to connect the Serial Console to the USB/JTAG interface. The Serial Console wont be available!");
		return;
	}

	useable_ = true;
}

void SerialConsole::registerCommands() { actuatorTest_.registerCommands(); }
