#include "../statemachine.h"

/*
 *	Private Variables
 */
//! \brief Keeps track of the current state we are in
uint8_t currentState_ = 0;

/*
 *	Functions
 */
uint8_t getCurrentState(void) { return currentState_; }

void setCurrentState(const uint8_t newState) { currentState_ = newState; }
