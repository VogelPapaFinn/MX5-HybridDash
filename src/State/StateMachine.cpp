#include "State/StateMachine.hpp"

// C++ includes
#include <utility>

/*
 *   Public Functions Implementations
 */
StateMachine::StateMachine(State* p_startState, std::vector<Transition> transitions)
{
	currState_ = p_startState;
	transitions_ = std::move(transitions);

	registerTransitions();
}

void StateMachine::start() { currState_->enter(); }

/*
 *   Private Functions Implementations
 */
void StateMachine::registerTransitions()
{
	if (transitions_.empty()) {
		return;
	}

	// Handler, which executes a state transition when a signal was emitted
	auto handler = [](void* p_stateMachine, const char*, int32_t eventId, void*)
	{
		if (p_stateMachine == nullptr) {
			return;
		}
		auto stateMachine = static_cast<StateMachine*>(p_stateMachine);

		// Check for valid transition and execute it
		const auto event = static_cast<STATE_EVENT_ID>(eventId);
		for (auto& t : stateMachine->transitions_) {
			if (t.from != stateMachine->currState_) {
				continue;
			}

			if (t.signal != event) {
				continue;
			}

			stateMachine->currState_->quit();
			stateMachine->currState_ = t.to;
			stateMachine->currState_->enter();

			return;
		}

		return;
	};

	// Register all events
	for (auto& t : transitions_) {
		esp_event_handler_instance_register(STATE_EVENT_BASE, t.signal, handler, this, nullptr);
	}
}
