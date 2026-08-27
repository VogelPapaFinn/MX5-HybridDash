#pragma once

namespace CanFrameGroups
{
	enum GROUP
	{
		CONFIGURATION,
		SENSOR,
		WIFI,
	};

	enum CONFIGURATION
	{
		REGISTER_AT_MASTER,
		CONFIRM_CONFIGURATION,
		BAKE_CONFIGURATION,
		WAKE_UP,
		RESTART,
	};

	enum SENSOR
	{
		BROADCAST_DATA
	};

	enum WIFI
	{
		SET_MASTER_IP,
		SET_SSID,
		SET_PASSWORD,
		JOIN_WIFI,
		EXECUTE_UPDATE,
	};
}