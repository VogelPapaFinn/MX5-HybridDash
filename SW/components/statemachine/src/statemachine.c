#include "../statemachine.h"

/*
 *	Private Variables
 */
//! \brief Keeps track of the current state we are in
State_t currentState_ = STATE_INIT;

/*
 *	Functions
 */
State_t getCurrentState(void) { return currentState_; }

void setCurrentState(const State_t newState) { currentState_ = newState; }
