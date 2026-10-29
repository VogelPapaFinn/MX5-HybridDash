#pragma once

// Project includes
#include "Can.hpp"
#include "CanReceiver.hpp"
#include "StateMachine/State.hpp"

class WaitingForConfiguration : public State, public CanReceiver
{
public:
	WaitingForConfiguration(Can* p_can);

	void enter() override;

	void quit() override;

private:
	/*
	 *   Private Functions
	 */
	void handleCanFrame(const Can::Frame_t& frame) override;
};
