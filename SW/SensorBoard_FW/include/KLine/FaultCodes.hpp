#pragma once

// C++ includes
#include <map>
#include <string>
#include <unordered_map>

/*
 *   Public enums & Variables
 */
enum FAULT_CODE
{
	MASS_AIRFLOW_SENSOR,
	INLET_AIR_TEMPERATURE_SENSOR,
	COOLANT_TEMPERATURE_SENSOR,
	THROTTLE_POSITION_SENSOR,
	KNOCK_SENSOR,
	FRONT_LAMBDA_NO_ACTIVITY,
	FRONT_LAMBDA_FROZEN,
	CRANKSHAFT_SENSOR,
	IDLE_VALVE,

	ALTERNATOR_SIGNAL_FAULTY,
	ALTERNATOR_OVERVOLTAGE,
	ALTERNATOR_NOT_CHARGING,

	IMMOBILIZER_COM_FAULT,
	IMMOBILIZER_EEPROM_NOT_REGISTERED,
	IMMOBILIZER_CODE_NOT_REGISTERED,
	IMMOBILIZER_CODE_WRONG,
	IMMOBILIZER_ID_WRONG,
	IMMOBILIZER_READ_WRITE_FAULT,
	IMMOBILIZER_COM_COUNTER,
};

const std::unordered_map<FAULT_CODE, std::string> g_FAULT_CODE_DESCRIPTIONS = {
	// Switch tests
	{FAULT_CODE::MASS_AIRFLOW_SENSOR, "Voltage < 0.86V or > 4.90V"},
	{FAULT_CODE::INLET_AIR_TEMPERATURE_SENSOR, "Voltage not between 0.16V and 4.84V"},
	{FAULT_CODE::COOLANT_TEMPERATURE_SENSOR, "Voltage not between 0.20V and 4.95V"},
	{FAULT_CODE::THROTTLE_POSITION_SENSOR, "Voltage not between 0.10V and 4.77V (twice in a row)"},
	{FAULT_CODE::KNOCK_SENSOR, "Idle Voltage not between 1.25V and 3.75V (twice in a row)"},
	{FAULT_CODE::FRONT_LAMBDA_NO_ACTIVITY, "Voltage threshold never reached"},
	{FAULT_CODE::FRONT_LAMBDA_FROZEN, "Voltage not proportional to air"},
	{FAULT_CODE::CRANKSHAFT_SENSOR, "Engine cranking but no synchronization"},
	{FAULT_CODE::IDLE_VALVE, "Valve not responding to signal"},

	{FAULT_CODE::ALTERNATOR_SIGNAL_FAULTY, "Alternator Voltage <8.50V"},
	{FAULT_CODE::ALTERNATOR_OVERVOLTAGE, "Alterator Voltage >18.50V or Battery Voltage >15.90V"},
	{FAULT_CODE::ALTERNATOR_NOT_CHARGING, "Alternator Voltage >17.00V and Battery Voltage <10.90V"},

	{FAULT_CODE::IMMOBILIZER_COM_FAULT, "Communication with Immobilizer faulty"},
	{FAULT_CODE::IMMOBILIZER_EEPROM_NOT_REGISTERED, "Immobilizer EEPROM not registered"},
	{FAULT_CODE::IMMOBILIZER_CODE_NOT_REGISTERED, "Immobilizer Codeword not registered"},
	{FAULT_CODE::IMMOBILIZER_CODE_WRONG, "Immobilizer Codeword wrong"},
	{FAULT_CODE::IMMOBILIZER_ID_WRONG, "Immobilizer ID wrong"},
	{FAULT_CODE::IMMOBILIZER_READ_WRITE_FAULT, "Immobilizer Read/Write error"},
	{FAULT_CODE::IMMOBILIZER_COM_COUNTER, "Immobilizer communication counter error"},
};
