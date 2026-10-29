#pragma once

// Project includes
#include "Can.hpp"
#include "CanReceiver.hpp"
#include "Handler/GuiHandler.hpp"
#include "StateMachine/State.hpp"

class Operation : public State, public CanReceiver
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
	GuiHandler* guiHandler_ = nullptr;
};
