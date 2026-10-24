#pragma once

// Project includes
#include "Config.hpp"
#include "Driver/Display.hpp"
#include "MessageReceiver.hpp"
#include "StateMachine/State.hpp"
#include "StateMachine/Utils/TimeoutTimer.hpp"

// C++ includes
#include <vector>

/*
 *	Class implementation
 */
class ConfigureDisplays : public State, public MessageReceiver
{
public:
	ConfigureDisplays(Can* p_can, Config* p_config, std::vector<Display>* p_displays);

	void enter() override;

	void quit() override;

private:
	/*
	 *   Private Functions
	 */
	void enterNextState();
	void handleCanFrame(const Can::Frame_t& frame) override;

	// START_DISPLAY
	void waitForDisplay();
	void stopWaitingForDisplay();

	// SET_CAN_ID
	void setCanId();

	// SET_SCREEN
	void setScreen();

	// SET_ROTATION
	void setRotation();

	// FINISH_DISPLAY
	void saveConfiguration();

	/*
	 *   Private Task Functions
	 */
	static void canRxTask(void* p_param);

	/*
	 *   Private Variables
	 */
	enum SUBSTATE
	{
		UNDEFINED = 0,
		START_DISPLAY,
		SET_CAN_ID,
		SET_SCREEN,
		SET_ROTATION,
		FINISH_DISPLAY
	};
	uint8_t substate_ = UNDEFINED;

	Config* config_ = nullptr;
	std::vector<Display>* displays_ = nullptr;

	TimeoutTimer* timeoutTimer_ = nullptr;
	TaskHandle_t canRxTask_ = nullptr;

	std::vector<Display>::iterator currDisplay_;

	// START_DISPLAY
	TaskHandle_t waitingForDisplayTask_ = nullptr;
};
