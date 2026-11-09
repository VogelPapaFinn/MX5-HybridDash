#pragma once

// Project includes
#include "Tests.hpp"

// C++ includes
#include <cstdint>
#include <map>

/*
 *   Array
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
};
