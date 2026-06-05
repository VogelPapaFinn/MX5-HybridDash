#pragma once

namespace CanFrame
{
	enum GROUP
	{
		CONFIGURATION,
		SENSOR
	};

	enum CONFIGURATION
	{
		REGISTER_AT_MASTER,
		SET_ID,
		CONFIRM_ID,
		SET_SCREEN,
		WAKE_UP,
	};

	enum SENSOR
	{
		BROADCAST_DATA
	};
}