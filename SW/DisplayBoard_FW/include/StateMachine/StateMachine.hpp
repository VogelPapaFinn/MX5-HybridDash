#pragma once

// Project includes
#include "State.hpp"
#include "StateEvents.hpp"

// C++ includes
#include <vector>

class StateMachine
{
public:
	struct Transition
	{
		State* from = nullptr;
		STATE_EVENT_ID signal = STATE_EVENT_ID::UNKNOWN;
		State* to = nullptr;
	};

	StateMachine(State* p_startState, std::vector<Transition> transitions);

	void start();

private:
	/*
	 *   Private Functions
	 */
	void registerTransitions();

	/*
	 *   Private Variables
	 */
	State* currState_ = nullptr;
	std::vector<Transition> transitions_;
};
