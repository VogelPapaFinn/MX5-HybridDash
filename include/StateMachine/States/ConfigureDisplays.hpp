#pragma once

// Project includes
#include "Driver/Display.hpp"
#include "MessageReceiver.hpp"
#include "StateMachine/State.hpp"

// C++ includes
#include <vector>

/*
 *	Class implementation
 */
class ConfigureDisplays : public State, public MessageReceiver
{
public:
	ConfigureDisplays(Can* p_can, std::vector<Display>* p_displays);

	void enter() override;

	void quit() override;

private:
	/*
	 *   Private Functions
	 */
	void startTimeoutTimer();
	void stopTimeoutTimer();
	void startCanRxTask();
	void stopCanRxTask();

	void enterNextState();
	void handleCanFrame(const Can::Frame_t& frame);

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
		PREPARE_NEXT_DISPLAY,
		WAITING_FOR_DISPLAY,
		SET_CAN_ID,
		SET_SCREEN,
		SET_ROTATION,
		FINISHED
	};
	uint8_t substate_ = UNDEFINED;

	Can* can_ = nullptr;
	std::vector<Display>* displays_ = nullptr;

	TimerHandle_t timeoutTimer_ = nullptr;
	TaskHandle_t canRxTask_ = nullptr;

	std::vector<Display>::iterator currDisplay_ = displays_->begin();
};
