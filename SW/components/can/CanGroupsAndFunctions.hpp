#pragma once

namespace CanFrameGroups
{
	enum GROUP
	{
		UPDATE_DISPLAY = 0,
		CONFIGURE_DISPLAY,
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

	enum CONFIGURE_DISPLAY
	{
		ENTER_CONFIGURATION_MODE,
		SET_CAN_ID,
		SET_SCREEN,
		SET_ROTATION,
		SAVE_CONFIGURATION
	};

	enum SENSOR
	{
		BROADCAST_DATA
	};
} // namespace CanFrameGroups
