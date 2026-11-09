#include "SerialConsole/Commands/ActuatorTests.hpp"

// Project includes
#include "KLine/Tests/SwitchTest.hpp"
#include "KLine/Tests/Tests.hpp"

// espidf includes
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr uint8_t TEST_PID = 0x30;

/*
 *   Public Function Implementations
 */
ActuatorTests::ActuatorTests(KLine* p_kline)
{
	kline_ = p_kline;

	commands_ = {
		esp_console_cmd_t{.command = "list_tests",
						  .help = "Prints a list of all available tests",
						  .hint = "",
						  .func_w_context = [](void* p_ctx, int argc, char** p_argv) -> int
						  { return static_cast<ActuatorTests*>(p_ctx)->listTests(argc, p_argv); },
						  .context = this},

		esp_console_cmd_t{.command = "execute_switch_test",
						  .help = "Prints a list of all available tests",
						  .hint = "<test> <1 = on, 0 = off>",
						  .func_w_context = [](void* p_ctx, int argc, char** p_argv) -> int
						  { return static_cast<ActuatorTests*>(p_ctx)->executeSwitchTest(argc, p_argv); },
						  .context = this},
	};
}

void ActuatorTests::registerCommands()
{
	for (auto& command : commands_) {
		esp_console_cmd_register(&command);
	}
}

/*
 *   Private Function Implementations
 */
void ActuatorTests::handleCommandAnswer(const KLineMessage& message)
{
	const auto payload = message.getPayload();

	if (payload.empty()) {
		return;
	}

	switch (payload.back()) {
		case 0x00: {
			printf("The ECU confirmed and executes the test!\n");
		} break;

		case 0x12: {
			printf("The ECU reported a wrong test number or wrong parameters!\n");
		} break;

		case 0x22: {
			printf("The ECU reported a wrong engine state!\n");
		} break;

		default:
			return;
	}
}

/*
 *   Command Function Implementations
 */
int ActuatorTests::listTests(int argc, char** p_argv)
{
	// List all switch tests
	printf("Switch tests:\n");
	for (const auto& test : g_SWITCH_TESTS) {
		printf("- %d %s\n", test.first, g_ECU_TEST_NAME.at(test.first).c_str());
	}

	return 0;
}

int ActuatorTests::executeSwitchTest(int argc, char** p_argv)
{
	if (argc < 3) {
		printf("Please provide the test number (see list_tests) and the value (1 = on, 0 = off)");
		return 1;
	}

	const std::string firstParameter = std::string{p_argv[1]};
	const std::string secondParameter = std::string{p_argv[2]};

	// Extract needed values
	const auto testNumber = static_cast<ECU_TEST>(std::stoi(firstParameter));
	volatile const uint8_t value = std::stoi(secondParameter) == 0 ? 0x00 : 0xFF;
	volatile const auto testId = g_SWITCH_TESTS.at(testNumber);

	// Tell the ECU to start the test
	KLineMessage message(TEST_PID, {testId, value, 0x00});
	kline_->send(message,
				 [this](KLineMessage& message)
				 {
					 printf("ECU answer: %s\n", message.toString().c_str());
					 this->handleCommandAnswer(message);
				 });

	return 0;
}
