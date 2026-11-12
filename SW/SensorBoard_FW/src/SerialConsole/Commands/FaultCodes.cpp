#include "SerialConsole/Commands/FaultCodes.hpp"

// C++ includes
#include <utility>

// Project includes
#include "KLine/FaultCodes.hpp"
#include "Sensors/SwSensor/NumericalSensors.hpp"

// espidf includes
#include "esp_log.h"

/*
 *   constexpr
 */
constexpr uint8_t TEST_PID = 0x30;

/*
 *   Public Function Implementations
 */
FaultCodes::FaultCodes(KLine* p_kline)
{
	kline_ = p_kline;

	commands_ = {
		esp_console_cmd_t{.command = "check_amount_faultcodes",
						  .help = "Checks how many fault codes the ECU currently reports",
						  .hint = "",
						  .func_w_context = [](void* p_ctx, int argc, char** p_argv) -> int
						  { return static_cast<FaultCodes*>(p_ctx)->checkAmountOfFaultCodes(argc, p_argv); },
						  .context = this},

		esp_console_cmd_t{.command = "read_faultcodes",
						  .help = "Reads all fault codes and prints their information",
						  .hint = "",
						  .func_w_context = [](void* p_ctx, int argc, char** p_argv) -> int
						  { return static_cast<FaultCodes*>(p_ctx)->readFaultCodes(argc, p_argv); },
						  .context = this},
	};
}

void FaultCodes::registerCommands()
{
	for (auto& command : commands_) {
		esp_console_cmd_register(&command);
	}
}

/*
 *   Private Function Implementations
 */
void FaultCodes::requestAmountOfFaultCodes(std::function<void(unsigned int)> cb)
{
	const auto sensor = getNumericalSensor(SENSOR::ECU_AMOUNT_FAULT_CODES);

	const auto upperPidByte = static_cast<uint8_t>((sensor.pid >> 8) & 0xFF);
	const auto lowerPidByte = static_cast<uint8_t>((sensor.pid >> 0) & 0xFF);

	KLineMessage message(0x22, {upperPidByte, lowerPidByte});
	kline_->send(message,
				 [cb, upperPidByte, lowerPidByte](KLineMessage& message)
				 {
					 // ERROR
					 if (message.getSid() == 0x7F) {
						 return;
					 }

					 // Check PID
					 auto payload = message.getPayload();
					 if (payload.size() <= 2 || payload.at(0) != upperPidByte || payload.at(1) != lowerPidByte) {
						 printf("Received the wrong PID as answer");

						 return;
					 }

					 if (cb) {
						 cb(payload.back());
					 }
				 });
}

/*
 *   Command Function Implementations
 */
int FaultCodes::checkAmountOfFaultCodes(int argc, char** p_argv)
{
	requestAmountOfFaultCodes([](unsigned int amountOfFaultCodes)
							  { printf("Amount of Fault Codes: %d", amountOfFaultCodes); });

	return 0;
}

int FaultCodes::readFaultCodes(int argc, char** p_argv)
{
	requestAmountOfFaultCodes(
		[this](unsigned int amountOfFaultCodes)
		{
			// Abort if there arent some
			if (amountOfFaultCodes == 0 && false) {
				return;
			}

			auto expectedResponseAmount = static_cast<uint8_t>(amountOfFaultCodes / 3);
			if (amountOfFaultCodes % 3 != 0) {
				expectedResponseAmount++;
			}

			KLineMessage message(0x13);
			kline_->send(
				message,
				[](std::vector<KLineMessage>& responses)
				{
					if (responses.empty()) {
						printf("No response\n");
					}

					for (const auto& m : responses) {
						if (m.getSid() != (0x13 + 0x40)) {
							continue;
						}

						// Wrong payload size
						const auto payload = m.getPayload();
						if (payload.size() < 6) {
							continue;
						}

						for (uint8_t i = 0; i < 5; i += 2) {
							const auto upperByte = payload.at(i);
							const auto lowerByte = payload.at(i + 1);

							const auto faultCode = static_cast<FAULT_CODE>((upperByte << 8) + lowerByte);

							printf("%d - %s", faultCode, g_FAULT_CODE_DESCRIPTIONS.at(faultCode).c_str());
							printf("\n");
						}
					}
				},
				expectedResponseAmount);
		});

	return 0;
}
