#pragma once

class State
{
public:
	virtual void enter() = 0;

	virtual void quit() = 0;

protected:
};
