#pragma once

// Project includes
#include "Sensors/Sensors.hpp"

struct NumericalSensor
{
	SENSOR id;
	uint16_t pid;
	uint8_t byteCount;
	double (*convertFunc)(uint16_t raw) = nullptr;
	const char* unit = "";
};

/*
 *   constexpr
 */
constexpr auto UNIT_PERCENT = "%";
constexpr auto UNIT_VOLT = "V";
constexpr auto UNIT_CELSIUS = "°C";
constexpr auto UNIT_MS = "ms";
constexpr auto UNIT_DEGREE = "°";

inline constexpr NumericalSensor SW_NUMERICAL_SENSORS[] = {
	{SENSOR::ECU_ALTERNATOR_LOAD_P, 0x17A5, 1,
	 [](const uint16_t raw) { return static_cast<double>(raw) * (100.0 / 256.0); }, UNIT_PERCENT},

	{SENSOR::ECU_ALTERNATOR_DESIRED_VOLT_V, 0x17A4, 1,
	 [](const uint16_t raw) { return static_cast<double>(raw) * (213.0 / 2560.0); }, UNIT_VOLT},

	{SENSOR::ECU_COOLANT_V, 0x1787, 2, [](const uint16_t raw) { return static_cast<double>(raw) * (5.0 / 1024.0); },
	 UNIT_VOLT},

	{SENSOR::ECU_COOLANT_C, 0x1729, 1, [](const uint16_t raw) { return static_cast<double>(raw - 64) * 0.625; },
	 UNIT_CELSIUS},

	{SENSOR::ECU_INJECTION_MS, 0x1762, 2, [](const uint16_t raw) { return static_cast<double>(raw) * 0.002; }, UNIT_MS},

	{SENSOR::ECU_IDLE_BYPASS_MS, 0x1769, 2, [](const uint16_t raw) { return static_cast<double>(raw) * 0.002; },
	 UNIT_MS},

	{SENSOR::ECU_ENGINE_LOAD_P, 0x174C, 2, [](const uint16_t raw) { return static_cast<double>(raw) / 40.96; },
	 UNIT_PERCENT},

	{SENSOR::ECU_AIRMASS_GS, 0x1750, 2, [](const uint16_t raw) { return static_cast<double>(raw) / 100.0; }, "g/s"},

	{SENSOR::ECU_PRE_CAT_LAMBDA_V, 0x1795, 1,
	 [](const uint16_t raw) { return static_cast<double>(raw) * (5.0 / 1024.0); }, UNIT_VOLT},

	{SENSOR::ECU_RPM, 0x1738, 2, [](const uint16_t raw) { return static_cast<double>(raw) * (1000 / 1024); }, "RPM"},

	{SENSOR::ECU_ADVANCED_IGNITION_D, 0x1743, 2,
	 [](const uint16_t raw) { return static_cast<double>(raw - 85) * (60.0 / 256.0); }, UNIT_DEGREE},

	{SENSOR::ECU_THROTTLE_POSITION_V, 0x1780, 2,
	 [](const uint16_t raw) { return static_cast<double>(raw) * (5.0 / 1024.0); }, UNIT_VOLT},

	{SENSOR::ECU_INPUT_V, 0x1785, 1, [](const uint16_t raw) { return static_cast<double>(raw) / 12.8; }, UNIT_VOLT},

	{SENSOR::ECU_SPEED, 0x174A, 1, [](const uint16_t raw) { return static_cast<double>(raw); }, "KMH"},

	{SENSOR::ECU_INTAKE_AIR_C, 0x1720, 1, [](const uint16_t raw) { return static_cast<double>(raw - 64) * 0.625; },
	 UNIT_CELSIUS},

	{SENSOR::ECU_INTAKE_AIR_V, 0x178A, 1, [](const uint16_t raw) { return static_cast<double>(raw) * (5.0 / 256.0); },
	 UNIT_VOLT},

	{SENSOR::ECU_MAF_V, 0x1790, 2, [](const uint16_t raw) { return static_cast<double>(raw) * (5.0 / 1024.0); },
	 UNIT_VOLT},

	{SENSOR::ECU_SHORT_FUEL_TRIM_P, 0x1754, 2,
	 [](const uint16_t raw) { return static_cast<double>(raw - 2048) * (100.0 / 4096.0); }, UNIT_PERCENT},

	{SENSOR::ECU_LONG_FUEL_TRIM_P, 0x1758, 1,
	 [](const uint16_t raw) { return static_cast<double>(raw - 128) * (100.0 / 256.0); }, UNIT_PERCENT},

	{SENSOR::ECU_KNOCK_RETARD_D, 0x1745, 1,
	 [](const uint16_t raw) { return static_cast<double>(raw) * (60.0 / 256.0); }, UNIT_DEGREE},

	{SENSOR::ECU_EGR_VALVE_POSITION, 0x1775, 1, [](const uint16_t raw) { return static_cast<double>(raw); }, "pos"},
};
