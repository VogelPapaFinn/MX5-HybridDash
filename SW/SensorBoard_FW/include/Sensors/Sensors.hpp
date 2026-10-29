#pragma once

// C++ includes
#include <string>
#include <unordered_map>

/*
 *   Public enums & Variables
 */
enum class SENSOR
{
	// Hardware Sensors
	FUEL_LEVEL_R = 0,
	FUEL_LEVEL_P,
	OIL_PRESSURE_R,
	OIL_PRESSURE_B,
	WATER_TEMP_R,
	WATER_TEMP_C,
	LEFT_INDICATOR,
	RIGHT_INDICATOR,
	SPEED,
	RPM,

	// Software Sensors

	END
};

const std::unordered_map<SENSOR, std::string> g_SENSOR_NAME = {
	{SENSOR::FUEL_LEVEL_R, "Fuel Level (Ohm)"},
	{SENSOR::FUEL_LEVEL_P, "Fuel Level (%)"},
	{SENSOR::OIL_PRESSURE_R, "Oil Pressure (Ohm)"},
	{SENSOR::OIL_PRESSURE_B, "Oil Pressure (Bool)"},
	{SENSOR::WATER_TEMP_R, "Water Temp (Ohm)"},
	{SENSOR::WATER_TEMP_C, "Water Temp (°C)"},
	{SENSOR::LEFT_INDICATOR, "Left Indicator"},
	{SENSOR::RIGHT_INDICATOR, "Right Indicator"},
	{SENSOR::SPEED, "Speed"},
	{SENSOR::RPM, "RPM"},
};
