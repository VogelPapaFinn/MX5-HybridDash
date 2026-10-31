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

	// Software Sensors - Numerical
	ECU_ALTERNATOR_LOAD_P,
	ECU_ALTERNATOR_DESIRED_VOLT_V,
	ECU_COOLANT_V,
	ECU_COOLANT_C,
	ECU_INJECTION_MS,
	ECU_IDLE_BYPASS_MS,
	ECU_ENGINE_LOAD_P,
	ECU_AIRMASS_GS,
	ECU_PRE_CAT_LAMBDA_V,
	ECU_RPM,
	ECU_ADVANCED_IGNITION_D,
	ECU_THROTTLE_POSITION_V,
	ECU_INPUT_V,
	ECU_SPEED,
	ECU_INTAKE_AIR_C,
	ECU_INTAKE_AIR_V,
	ECU_MAF_V,
	ECU_SHORT_FUEL_TRIM_P,
	ECU_LONG_FUEL_TRIM_P,
	ECU_KNOCK_RETARD_D,
	ECU_EGR_VALVE_POSITION,

	// Software Sensors - Bitfield
	ECU_BRAKE_ON,
	ECU_FAN_SPEED_SLOW,
	ECU_FAN_SPEED_FAST,
	ECU_FUEL_PUMP_ON,
	ECU_LOW_VOLTAGE_LIGHT_ON,
	ECU_CHECK_ENGINE_LIGHT_ON,
	ECU_PRESSURE_SWITCH_POWER_STEERING_ON,
	ECU_AC_ON,
	ECU_CLUTCH_ENGAGED,
	ECU_TEN_BRIDGED,
	ECU_AC_COMPRESSOR_ON,
	ECU_LAMBDA_HEATING_ON,
	ECU_FPRC_ON,

	END
};

const std::unordered_map<SENSOR, std::string> g_SENSOR_NAME = {
	// Hardware Sensors
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

	// Software Sensors - Numerical
	{SENSOR::ECU_ALTERNATOR_LOAD_P, "Alternator Load (%)"},
	{SENSOR::ECU_ALTERNATOR_DESIRED_VOLT_V, "Alternator Desired Voltage (V)"},
	{SENSOR::ECU_COOLANT_V, "Coolant Temperature (V)"},
	{SENSOR::ECU_COOLANT_C, "Coolant Temperature (C)"},
	{SENSOR::ECU_INJECTION_MS, "Injection (ms)"},
	{SENSOR::ECU_IDLE_BYPASS_MS, "Idle Bypass (ms)"},
	{SENSOR::ECU_ENGINE_LOAD_P, "Engine Load (%)"},
	{SENSOR::ECU_AIRMASS_GS, "Airmass (G/S)"},
	{SENSOR::ECU_PRE_CAT_LAMBDA_V, "Pre-Cat Lambda (V)"},
	{SENSOR::ECU_RPM, "RPM"},
	{SENSOR::ECU_ADVANCED_IGNITION_D, "Advanced Ignition (°)"},
	{SENSOR::ECU_THROTTLE_POSITION_V, "Throttle Position (V)"},
	{SENSOR::ECU_INPUT_V, "ECU Input Voltage (V)"},
	{SENSOR::ECU_SPEED, "Speed (kmh)"},
	{SENSOR::ECU_INTAKE_AIR_C, "Intake Air Temperature (C)"},
	{SENSOR::ECU_INTAKE_AIR_V, "Intake Air Temperature (V)"},
	{SENSOR::ECU_MAF_V, "MAF Voltage (V)"},
	{SENSOR::ECU_SHORT_FUEL_TRIM_P, "Shorttime Fuel Trim (%)"},
	{SENSOR::ECU_LONG_FUEL_TRIM_P, "Longtime Fuel Trim (%)"},
	{SENSOR::ECU_KNOCK_RETARD_D, "Knock Retard (°)"},
	{SENSOR::ECU_EGR_VALVE_POSITION, "EGR Valve Position"},

	// Software Sensors - Bitfield
	{SENSOR::ECU_BRAKE_ON, "Brake Pedal pressed"},
	{SENSOR::ECU_FAN_SPEED_SLOW, "Fan active - Slow"},
	{SENSOR::ECU_FAN_SPEED_FAST, "Fan active - Fast"},
	{SENSOR::ECU_FUEL_PUMP_ON, "Fuel Pump on"},
	{SENSOR::ECU_LOW_VOLTAGE_LIGHT_ON, "Battery Low Voltage"},
	{SENSOR::ECU_CHECK_ENGINE_LIGHT_ON, "Check Engine Light"},
	{SENSOR::ECU_PRESSURE_SWITCH_POWER_STEERING_ON, "Pressure Switch of Power Steering on"},
	{SENSOR::ECU_AC_ON, "AC on"},
	{SENSOR::ECU_CLUTCH_ENGAGED, "Clutch pedal pressed"},
	{SENSOR::ECU_TEN_BRIDGED, "TEN bridged"},
	{SENSOR::ECU_AC_COMPRESSOR_ON, "AC Compressor on"},
	{SENSOR::ECU_LAMBDA_HEATING_ON, "Lambda Heating on"},
	{SENSOR::ECU_FPRC_ON, "FPRC on"},
};
