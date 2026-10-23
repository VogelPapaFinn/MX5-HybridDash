#pragma once

// Project includes
#include "Can.hpp"
#include "Handler/GuiHandler.hpp"
#include "MessageReceiver.hpp"
#include "StateMachine/State.hpp"

class Operation : public State, public MessageReceiver
{
public:
	Operation(Can* p_can, GuiHandler* p_guiHandler);

	void enter() override;

	void quit() override;

private:
	/*
	 *   Private Functions
	 */
	void handleCanFrame(const Can::Frame_t& frame) override;

	/*
	 *   Private Variables
	 */
	Can* can_ = nullptr;
	GuiHandler* guiHandler_ = nullptr;
};
