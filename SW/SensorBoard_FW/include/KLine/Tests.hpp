#pragma once

// C++ includes
#include <map>
#include <string>
#include <unordered_map>

/*
 *   Public enums & Variables
 */
enum ECU_TEST
{
	// Switch Tests
	FAN_AC_ON,
	FAN_ON,
	TANK_BLEEDING_VALVE,
	FUEL_PUMP,
	FUEL_PRESSURE_VALVE,
	SHUTDOWN_CYCLINDER_1,
	SHUTDOWN_CYCLINDER_2,
	SHUTDOWN_CYCLINDER_3,
	SHUTDOWN_CYCLINDER_4,
	HEATING_FRONT_LAMBDA,
	AC_COMPRESSOR,
	VOLTAGE_BULB,
	TURN_OFF_ALTERNATOR,

	// Value Tests
	ALTER_INJECTION_AMOUNT,
	ALTER_IGNITION_TIMING,
	ALTER_IDLE_VALVE_POS,
	DRIVE_EGR_TO_POS,
};

const std::unordered_map<ECU_TEST, std::string> g_ECU_TEST_NAME = {
	// Switch tests
	{ECU_TEST::FAN_AC_ON, "Turn on the AC cooling fan"},
	{ECU_TEST::FAN_ON, "Turn on the cooling fan"},
	{ECU_TEST::TANK_BLEEDING_VALVE, "Open the bleeding valve in the fuel tank"},
	{ECU_TEST::FUEL_PUMP, "Turn on the fuel pump"},
	{ECU_TEST::FUEL_PRESSURE_VALVE, "Turn on the fuel pressure valve"},
	{ECU_TEST::SHUTDOWN_CYCLINDER_1, "Shutdown the 1th Cyclinder (only when value = 0)"},
	{ECU_TEST::SHUTDOWN_CYCLINDER_2, "Shutdown the 2nd Cyclinder (only when value = 0)"},
	{ECU_TEST::SHUTDOWN_CYCLINDER_3, "Shutdown the 3rd Cyclinder (only when value = 0)"},
	{ECU_TEST::SHUTDOWN_CYCLINDER_4, "Shutdown the 4th Cyclinder (only when value = 0)"},
	{ECU_TEST::HEATING_FRONT_LAMBDA, "Turn on the heating of the front lambda sensor"},
	{ECU_TEST::AC_COMPRESSOR, "Turn on the AC compressor"},
	{ECU_TEST::VOLTAGE_BULB, "Toggle the Voltage Bulb in the Dashboard"},
	{ECU_TEST::TURN_OFF_ALTERNATOR, "Turn off the alternator (only when value = 0)"},

	// Value tests
	{ECU_TEST::ALTER_INJECTION_AMOUNT, "Change the amount of injected fuel (-50% <= value <= 50%)"},
	{ECU_TEST::ALTER_IGNITION_TIMING, "Change the ignition timing (-30° <= value <= 30°; default value = 0°)"},
	{ECU_TEST::ALTER_IDLE_VALVE_POS,
	 "Set idle valve position (0% <= value <= 100%, default value is around 18 when the engine is warm)"},
	{ECU_TEST::DRIVE_EGR_TO_POS, "Move the EGR to the specific position (0 <= value <= 255)"},
};

/*
 *   Switch Tests
 */
inline const std::map<ECU_TEST, uint8_t> g_SWITCH_TESTS = {
	{ECU_TEST::FAN_AC_ON, 0x3A},
	{ECU_TEST::FAN_ON, 0x3C},

	{ECU_TEST::TANK_BLEEDING_VALVE, 0x10},

	{ECU_TEST::FUEL_PUMP, 0x23},
	{ECU_TEST::FUEL_PRESSURE_VALVE, 0x20},

	{ECU_TEST::SHUTDOWN_CYCLINDER_1, 0x01},
	{ECU_TEST::SHUTDOWN_CYCLINDER_2, 0x02},
	{ECU_TEST::SHUTDOWN_CYCLINDER_3, 0x03},
	{ECU_TEST::SHUTDOWN_CYCLINDER_4, 0x04},

	{ECU_TEST::HEATING_FRONT_LAMBDA, 0x35},

	{ECU_TEST::AC_COMPRESSOR, 0x38},

	{ECU_TEST::VOLTAGE_BULB, 0x44},

	{ECU_TEST::TURN_OFF_ALTERNATOR, 0x3E},
};

/*
 *   Value Tests
 */
inline const std::map<ECU_TEST, uint8_t> g_VALUE_TESTS = {
	{ECU_TEST::ALTER_INJECTION_AMOUNT, 0x00},
	{ECU_TEST::ALTER_IGNITION_TIMING, 0x0A},
	{ECU_TEST::ALTER_IDLE_VALVE_POS, 0x0C},
	{ECU_TEST::DRIVE_EGR_TO_POS, 0x4F},
};
