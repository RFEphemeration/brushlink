#pragma once
#ifndef BRUSHLINK_COMMAND_H
#define BRUSHLINK_COMMAND_H

#include "Action.h"

namespace Brushlink
{

// forward declare for Evaluate parameter, maybe not necessary
struct Unit;

// temporary
struct CommandEvaluation
{
	Action_Event action_event;
	bool finished;

};

// temporary
struct CommandContext
{

};

// temporary
struct Command
{
	CommandEvaluation Evaluate(CommandContext & context, Unit & unit) { return {}; }
	bool EvaluateEveryTick() { return false; }
};


} // namespace Brushlink

#endif // BRUSHLINK_COMMAND_H
