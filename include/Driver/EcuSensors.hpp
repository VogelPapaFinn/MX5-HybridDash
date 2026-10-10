#pragma once

// Project includes
#include "EcuPids.h"

// C++ includes
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>

/*
 *	constexpr
 */
constexpr auto UNIT_PERCENT = "%";
constexpr auto UNIT_VOLT = "V";
constexpr auto UNIT_CELSIUS = "°C";
constexpr auto UNIT_MS = "ms";
constexpr auto UNIT_DEGREE = "°";

constexpr auto BITMASK_FUNCTION = [](const uint32_t raw) { return raw > 0 ? 1.0 : 0.0; };

/*
 *	Use namespace
 */
using namespace ECU;

/*
 *	Public Struct
 */
struct EcuSensor
{
	uint16_t id;
	std::string name;

	uint8_t responseByteCount = 1;
	std::string unit = "";
	uint8_t bitInMask = 0;

	uint16_t rawValue = 0;
	std::function<double(uint16_t)> convertRawValue;

	double getConvertedValue() const
	{
		if (convertRawValue) {
			return convertRawValue(rawValue);
		}

		return 0.0;
	}

	std::string toJson() const
	{
		std::stringstream output;
		output << "{";
		output << "\"id\":" << "\"" << id << "\",";
		output << "\"name\":" << "\"" << name << "\",";
		output << "\"value\":" << "\"" << getConvertedValue() << "\",";
		output << "\"unit\":" << "\"" << unit << "\"";
		output << "}";
		return output.str();
	}
};

/*
 *	Public ECU Sensor Map
 */
// TODO: Find out the bits of the different bitmask entries
inline std::unordered_map<uint16_t, EcuSensor> ECU_SENSORS{
	{ALTERNATOR_LOAD_P,
	 EcuSensor{.id = ALTERNATOR_LOAD_P, .name = "Alternator - Load", .responseByteCount = 1, .unit = UNIT_PERCENT,
	           .convertRawValue = [](const uint16_t raw) { return raw / 2.55; }}},

	{ALTERNATOR_DESIRED_VOLTAGE,
	 EcuSensor{.id = ALTERNATOR_DESIRED_VOLTAGE, .name = "Alternator - Desired Voltage", .responseByteCount = 1,
	           .unit = UNIT_VOLT,
	           .convertRawValue = [](const uint16_t raw) { return raw / 10; }}},

	{CABIN_FAN_ON,
	 EcuSensor{.id = CABIN_FAN_ON, .name = "Cabin Fan - On", .responseByteCount = 1, .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{BRAKE_ON,
	 EcuSensor{.id = BRAKE_ON, .name = "Brake - On", .responseByteCount = 1, .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{REAR_DEFROSTER_ON,
	 EcuSensor{.id = REAR_DEFROSTER_ON, .name = "Rear Window Defroster - On", .responseByteCount = 1,
	           .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{DAYTIME_LIGHTS_ON,
	 EcuSensor{.id = DAYTIME_LIGHTS_ON, .name = "Daytime Lights - On", .responseByteCount = 1, .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{COOLANT_V,
	 EcuSensor{.id = COOLANT_V, .name = "Coolant - Sensor Voltage", .responseByteCount = 2, .unit = UNIT_VOLT,
	           .convertRawValue = [](const uint16_t raw) { return static_cast<double>(raw) * (5.0 / 1023.0); }}},

	{COOLANT_C,
	 EcuSensor{.id = COOLANT_C, .name = "Coolant - Degree", .responseByteCount = 1, .unit = UNIT_CELSIUS,
	           .convertRawValue = [](const uint16_t raw) { return static_cast<double>(raw - 40 - 32) / 1.8; }}},

	{EGR_ON,
	 EcuSensor{.id = EGR_ON, .name = "EGR - On", .responseByteCount = 1, .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{FAN_SPEED_SLOW,
	 EcuSensor{.id = FAN_SPEED_SLOW, .name = "Cooling Fan Speed - On, Slow Speed", .responseByteCount = 1,
	           .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{FAN_SPEED_MEDIUM,
	 EcuSensor{.id = FAN_SPEED_MEDIUM, .name = "Cooling Fan Speed - On, Medium Speed", .responseByteCount = 1,
	           .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{FAN_SPEED_FAST,
	 EcuSensor{.id = FAN_SPEED_FAST, .name = "Cooling Fan Speed - On, Fast Speed", .responseByteCount = 1,
	           .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{FUEL_PUMP_ON,
	 EcuSensor{.id = FUEL_PUMP_ON, .name = "Fuel Pump - On", .responseByteCount = 1, .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	// TODO: Find out the scale of the injection time
	{INJECTION_MS,
	 EcuSensor{.id = INJECTION_MS, .name = "Injector - Injection Time", .responseByteCount = 2, .unit = UNIT_MS,
	           .convertRawValue = [](const uint16_t raw) { return raw / 1000.0; }}},

	{LOW_VOLTAGE_LIGHT_ON,
	 EcuSensor{.id = LOW_VOLTAGE_LIGHT_ON, .name = "Battery - Low Voltage (Light)", .responseByteCount = 1,
	           .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{HEADLIGHT_LEFT_ON,
	 EcuSensor{.id = HEADLIGHT_LEFT_ON, .name = "Headlight Left - On", .responseByteCount = 1, .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{HEADLIGHT_RIGHT_ON,
	 EcuSensor{.id = HEADLIGHT_RIGHT_ON, .name = "Headlight Right - On", .responseByteCount = 1,
	           .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{IDLE_BYPASS_MS,
	 EcuSensor{.id = IDLE_BYPASS_MS, .name = "Idle Bypass - Time", .responseByteCount = 2, .unit = UNIT_MS,
	           .convertRawValue = [](const uint16_t raw) { return raw * 0.002; }}},

	{IDLE_SWITCH_ON,
	 EcuSensor{.id = IDLE_SWITCH_ON, .name = "Idle Switch - On", .responseByteCount = 1, .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{ENGINE_LOAD_P,
	 EcuSensor{.id = ENGINE_LOAD_P, .name = "Engine - Load", .responseByteCount = 2, .unit = UNIT_PERCENT,
	           .convertRawValue = [](const uint16_t raw) { return raw / 40.96; }}},

	{AIR_MASS_V,
	 EcuSensor{.id = AIR_MASS_V, .name = "Airmass", .responseByteCount = 2, .unit = UNIT_VOLT,
	           .convertRawValue = [](const uint16_t raw) { return raw * (5 / 1023); }}},

	// TODO: Test if "calculation" is correct
	{AIR_MASS_GS,
	 EcuSensor{.id = AIR_MASS_GS, .name = "Airmass", .responseByteCount = 2, .unit = "g/s",
	           .convertRawValue = [](const uint16_t raw) { return raw; }}},

	{MAIN_RELAY_ON,
	 EcuSensor{.id = MAIN_RELAY_ON, .name = "Main Relais - On", .responseByteCount = 1, .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{CHECK_ENGINE_LIGHT,
	 EcuSensor{.id = CHECK_ENGINE_LIGHT, .name = "Engine - Check Engine Light", .responseByteCount = 1,
	           .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{PRE_CAT_LAMBDA_V,
	 EcuSensor{.id = PRE_CAT_LAMBDA_V, .name = "Airmass Pre-Cat - Voltage", .responseByteCount = 2, .unit = UNIT_VOLT,
	           .convertRawValue = [](const uint16_t raw) { return raw * (5 / 1023); }}},

	{PRESSURE_SWITCH_POWER_STEERING_ON,
	 EcuSensor{.id = PRESSURE_SWITCH_POWER_STEERING_ON, .name = "Power Steering - Pressure Switch On",
	           .responseByteCount = 1,
	           .convertRawValue = [](const uint16_t raw) { return raw > 0 ? 1.0 : 0.0; }}},

	{RPM,
	 EcuSensor{.id = RPM, .name = "RPM", .responseByteCount = 2,
	           .convertRawValue = [](const uint16_t raw) { return raw; }}},

	{EGR_VALVE_POSITION,
	 EcuSensor{.id = EGR_VALVE_POSITION, .name = "EGR - Valve Position", .responseByteCount = 1,
	           .convertRawValue = [](const uint16_t raw) { return raw; }}},

	{ADVANCING_IGNITION_DEG,
	 EcuSensor{.id = ADVANCING_IGNITION_DEG, .name = "Ignition Timing - Advancing", .responseByteCount = 2,
	           .unit = UNIT_DEGREE,
	           .convertRawValue = [](const uint16_t raw) { return (raw - 255) / 12.8; }}},

	{IMMOBILIZER_ON,
	 EcuSensor{.id = IMMOBILIZER_ON, .name = "Immobilizer - On", .responseByteCount = 1, .bitInMask = 0b00000001,
	           .convertRawValue = BITMASK_FUNCTION}},

	{THROTTLE_POSITION_V,
	 EcuSensor{.id = THROTTLE_POSITION_V, .name = "Throttle - Voltage", .responseByteCount = 2, .unit = UNIT_VOLT,
	           .convertRawValue = [](const uint16_t raw) { return raw * (5 / 1023); }}},

	{ECU_INPUT_VOLTAGE,
	 EcuSensor{.id = ECU_INPUT_VOLTAGE, .name = "ECU - Input Voltage", .responseByteCount = 1, .unit = UNIT_VOLT,
	           .convertRawValue = [](const uint16_t raw) { return (raw / 12.8) + 1; }}},

	{SPEED_KMH,
	 EcuSensor{.id = SPEED_KMH, .name = "Speed", .responseByteCount = 1, .unit = "KMH",
	           .convertRawValue = [](const uint16_t raw) { return raw; }}},
};
