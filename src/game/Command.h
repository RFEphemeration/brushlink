#pragma once
#ifndef BRUSHLINK_COMMAND_H
#define BRUSHLINK_COMMAND_H

#include "Action.h"

namespace Brushlink
{

// forward declare for Evaluate parameter, maybe not necessary
struct Unit;

// temporary
struct CommandContext
{
	Game & game;
	PlayerID player_id;

	shared_ptr<CommandContext> parent;
};

// temporary
struct Action_Command
{
	Action_Step Evaluate(CommandContext & context, Unit & unit) { return {}; }
	bool EvaluateEveryTick() { return false; }
};



} // namespace Brushlink

#endif // BRUSHLINK_COMMAND_H
