#pragma once

// Project includes
#include "Tests.hpp"

// C++ includes
#include <cstdint>

struct SwitchTest
{
	ECU_TEST type;
	uint8_t testId = 0;
};

/*
 *   Array
 */
inline constexpr SwitchTest SWITCH_TESTS[] = {
	{ECU_TEST::FAN_MAX, 0x3A},
	{ECU_TEST::FAN_MED, 0x3C},

	{ECU_TEST::FUEL_PUMP, 0x23},
	{ECU_TEST::FUEL_PRESSURE_VALVE, 0x20},

	{ECU_TEST::SHUTDOWN_CYCLINDER_1, 0x01},
	{ECU_TEST::SHUTDOWN_CYCLINDER_2, 0x02},
	{ECU_TEST::SHUTDOWN_CYCLINDER_3, 0x03},
	{ECU_TEST::SHUTDOWN_CYCLINDER_4, 0x04},

	{ECU_TEST::HEATING_FRONT_IAT, 0x35},

	{ECU_TEST::AC_COMPRESSOR, 0x38},

	{ECU_TEST::VOLTAGE_BULB, 0x44},
};
