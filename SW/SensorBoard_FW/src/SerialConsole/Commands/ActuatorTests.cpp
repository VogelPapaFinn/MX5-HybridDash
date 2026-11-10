#include "SerialConsole/Commands/ActuatorTests.hpp"

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
						  .help = "Executes a switch test",
						  .hint = "<test> <value: 1 = on, 0 = off>",
						  .func_w_context = [](void* p_ctx, int argc, char** p_argv) -> int
						  { return static_cast<ActuatorTests*>(p_ctx)->executeSwitchTest(argc, p_argv); },
						  .context = this},

		esp_console_cmd_t{.command = "execute_value_test",
						  .help = "Executes a value test",
						  .hint = "<test> <value, see list_tests to see allowed values>",
						  .func_w_context = [](void* p_ctx, int argc, char** p_argv) -> int
						  { return static_cast<ActuatorTests*>(p_ctx)->executeValueTest(argc, p_argv); },
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

int ActuatorTests::getValueTestValue(const ECU_TEST ecuTest, const int passedValue)
{
	switch (ecuTest) {
		case ALTER_INJECTION_AMOUNT: {
			if (passedValue < -50 || passedValue > 50) {
				printf("Faulty injection amount in %% (-50 <= value <= 50)");
				return -1;
			}

			const auto convertedValue = static_cast<uint8_t>(128.0f + static_cast<float>(passedValue) * 2.56f);

			return convertedValue > 255 ? 255 : convertedValue;
		} break;

		case ALTER_IGNITION_TIMING: {
			if (passedValue < -30 || passedValue > 30) {
				printf("Faulty ignition timing (-30 <= value <= 30)");
				return -1;
			}

			const auto convertedValue = static_cast<uint8_t>(128.0f + static_cast<float>(passedValue) / 0.234f);

			return convertedValue > 255 ? 255 : convertedValue;
		} break;

		case ALTER_IDLE_VALVE_POS: {
			if (passedValue < 0 || passedValue > 100) {
				printf("Faulty idle valve position (0 <= value <= 100)");
				return -1;
			}

			auto convertedValue = static_cast<uint8_t>(static_cast<float>(passedValue) * 2.56f);

			return convertedValue > 255 ? 255 : convertedValue;
		} break;

		default:
			return 0;
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

	// List all value tests
	printf("\n");
	printf("Value tests:\n");
	for (const auto& test : g_VALUE_TESTS) {
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
	const uint8_t value = std::stoi(secondParameter) == 0 ? 0x00 : 0xFF;

	// Check if test exists
	if (g_SWITCH_TESTS.find(testNumber) == g_SWITCH_TESTS.end()) {
		printf("This switch test does not exist!");
		return 1;
	}

	// Tell the ECU to start the test
	KLineMessage message(TEST_PID, {g_SWITCH_TESTS.at(testNumber), value, 0x00});
	kline_->send(message,
				 [this](KLineMessage& message)
				 {
					 // printf("ECU answer: %s\n", message.toString().c_str());
					 this->handleCommandAnswer(message);
				 });

	return 0;
}

int ActuatorTests::executeValueTest(int argc, char** p_argv)
{
	if (argc < 3) {
		printf("Please provide the test number (see list_tests) and the value");
		return 1;
	}

	const std::string firstParameter = std::string{p_argv[1]};
	const std::string secondParameter = std::string{p_argv[2]};

	// Extract needed values
	const auto testNumber = static_cast<ECU_TEST>(std::stoi(firstParameter));
	const int value = std::stoi(secondParameter);

	// Check if test exists
	if (g_VALUE_TESTS.find(testNumber) == g_VALUE_TESTS.end()) {
		printf("This value test does not exist!");
		return 1;
	}

	// Get converted value
	const auto convertedValue = getValueTestValue(testNumber, value);
	if (convertedValue < 0 || convertedValue > 255) {
		printf("Faulty parameter (%d), aborting test!", convertedValue);
		return 1;
	}

	// Tell the ECU to start the test
	KLineMessage message(TEST_PID, {g_VALUE_TESTS.at(testNumber), static_cast<uint8_t>(convertedValue), 0x00});
	kline_->send(message,
				 [this](KLineMessage& message)
				 {
					 // printf("ECU answer: %s\n", message.toString().c_str());
					 this->handleCommandAnswer(message);
				 });

	return 0;
}
