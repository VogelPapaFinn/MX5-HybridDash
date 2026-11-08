#pragma once

// Project includes
#include "Tests.hpp"

// C++ includes
#include <cstdint>

struct SwitchTest
{
	ECU_TEST type;
	uint8_t testId = 0;
	uint8_t p1 = 0;
	uint8_t p2 = 0;
};

/*
 *   Array
 */
inline constexpr SwitchTest SW_BITFIELD_SENSORS[] = {
	{ECU_TEST::FAN_MAX, 0x3A, 0xFF, 0x00},
	{ECU_TEST::FAN_MED, 0x3C, 0xFF, 0x00},

	{ECU_TEST::FUEL_PUMP, 0x23, 0xFF, 0x00},
	{ECU_TEST::FUEL_PRESSURE_VALVE, 0x20, 0xFF, 0x00},

	{ECU_TEST::SHUTDOWN_CYCLINDER_1, 0x01, 0x00, 0x00},
	{ECU_TEST::SHUTDOWN_CYCLINDER_2, 0x02, 0x00, 0x00},
	{ECU_TEST::SHUTDOWN_CYCLINDER_3, 0x03, 0x00, 0x00},
	{ECU_TEST::SHUTDOWN_CYCLINDER_4, 0x04, 0x00, 0x00},

	{ECU_TEST::HEATING_FRONT_IAT, 0x35, 0xFF, 0x00},

	{ECU_TEST::AC_COMPRESSOR, 0x38, 0xFF, 0x00},

	{ECU_TEST::VOLTAGE_BULB_ON, 0x44, 0xFF, 0x00},
	{ECU_TEST::VOLTAGE_BULB_OFF, 0x44, 0x00, 0x00},
};
