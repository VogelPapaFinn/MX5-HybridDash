#pragma once

// C++ includes
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
};
