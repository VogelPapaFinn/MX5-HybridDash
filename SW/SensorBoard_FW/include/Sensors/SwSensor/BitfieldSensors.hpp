#pragma once

// Project includes
#include "Sensors/Sensors.hpp"

struct BitfieldSensor
{
	SENSOR id;
	uint16_t pid;
	uint8_t bitMask;
};

/*
 *   Helper Function
 */
inline bool getBitValue(const uint8_t bitField, const BitfieldSensor& sensor) { return bitField & sensor.bitMask; }

/*
 *   Array
 */
inline constexpr BitfieldSensor SW_BITFIELD_SENSORS[] = {
	{SENSOR::ECU_BRAKE_ON, 0x1701, 0b00000100},

	{SENSOR::ECU_FAN_SPEED_SLOW, 0x1711, 0b00010000},

	{SENSOR::ECU_FAN_SPEED_FAST, 0x1711, 0b00000100},

	{SENSOR::ECU_FUEL_PUMP_ON, 0x1718, 0b00000001},

	{SENSOR::ECU_LOW_VOLTAGE_LIGHT_ON, 0x1713, 0b00001000},

	{SENSOR::ECU_CHECK_ENGINE_LIGHT_ON, 0x1711, 0b00000001},

	{SENSOR::ECU_PRESSURE_SWITCH_POWER_STEERING_ON, 0x1701, 0b00010000},

	{SENSOR::ECU_EGR_VALVE_POSITION, 0x1775, 0b00000001},

	{SENSOR::ECU_AC_ON, 0x1701, 0b00001000},

	{SENSOR::ECU_CLUTCH_ENGAGED, 0x1702, 0b00010000},

	{SENSOR::ECU_CLUTCH_ENGAGED, 0x1705, 0b00000001},

	{SENSOR::ECU_AC_COMPRESSOR_ON, 0x1711, 0b00000010},

	{SENSOR::ECU_LAMBDA_HEATING_ON, 0x1711, 0b00100000},

	{SENSOR::ECU_FPRC_ON, 0x1712, 0b00000001},
};
