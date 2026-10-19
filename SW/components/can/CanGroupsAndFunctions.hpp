#pragma once

namespace CanFrameGroups
{
	enum GROUP
	{
		DISPLAY_UPDATE = 0,
		CONFIGURATION,
		SENSOR,
	};

	enum DISPLAY_UPDATE
	{
		ENTER_UPDATE_MODE,
		SET_SSID,
		CONFIRM_SSID,
		SET_PSSWD,
		CONFIRM_PSSWD,
		SET_HOST_IP,
		CONFIRM_HOST_IP,
		JOIN_WIFI,
		EXECUTE_UPDATE,
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
} // namespace CanFrameGroups
