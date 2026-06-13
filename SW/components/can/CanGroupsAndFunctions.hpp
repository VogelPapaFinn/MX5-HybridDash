#pragma once

namespace CanFrame
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
		SET_ID,
		CONFIRM_ID,
		SET_SCREEN,
		SET_ROTATION,
		CONFIRM_CONFIGURATION,
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