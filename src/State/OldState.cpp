#include "State/OldState.hpp"

/*
 *	Public Function Implementations
 */
OldState::OldState(TYPE type) { type_ = type; }

OldState::TYPE OldState::getType() const { return type_; }
