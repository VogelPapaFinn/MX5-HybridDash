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
	FAN_MAX,
	FAN_MED,
	FUEL_PUMP,
	FUEL_PRESSURE_VALVE,
	SHUTDOWN_CYCLINDER_1,
	SHUTDOWN_CYCLINDER_2,
	SHUTDOWN_CYCLINDER_3,
	SHUTDOWN_CYCLINDER_4,
	HEATING_FRONT_IAT,
	AC_COMPRESSOR,
	VOLTAGE_BULB,
};

const std::unordered_map<ECU_TEST, std::string> g_ECU_TEST_NAME = {
	// Switch tests
	{ECU_TEST::FAN_MAX, "Turn on the cooling fan (MAX speed)"},
	{ECU_TEST::FAN_MED, "Turn on the cooling fan (LOW speed)"},
	{ECU_TEST::FUEL_PUMP, "Turn on the fuel pump"},
	{ECU_TEST::FUEL_PRESSURE_VALVE, "Turn on the fuel pressure valve"},
	{ECU_TEST::SHUTDOWN_CYCLINDER_1, "Shutdown the 1th Cyclinder"},
	{ECU_TEST::SHUTDOWN_CYCLINDER_2, "Shutdown the 2nd Cyclinder"},
	{ECU_TEST::SHUTDOWN_CYCLINDER_3, "Shutdown the 3rd Cyclinder"},
	{ECU_TEST::SHUTDOWN_CYCLINDER_4, "Shutdown the 4th Cyclinder"},
	{ECU_TEST::HEATING_FRONT_IAT, "Turn on the heating of the fron IAT sensor"},
	{ECU_TEST::AC_COMPRESSOR, "Turn on the AC compressor"},
	{ECU_TEST::VOLTAGE_BULB, "Toggle the Voltage Bulb in the Dashboard"},
};
