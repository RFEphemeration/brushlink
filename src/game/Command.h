#pragma once
#ifndef BRUSHLINK_COMMAND_H
#define BRUSHLINK_COMMAND_H

#include "Action.h"

namespace Command
{
	struct Context;
} // namespace Command

namespace Brushlink
{

// forward declare for Evaluate parameter, maybe not necessary
struct Unit;
struct Game;

// temporary
/*
struct CommandContext
{
	Game & game;
	PlayerID player_id;

	std::shared_ptr<CommandContext> parent;
};
*/

// temporary
struct Action_Command
{
	Action_Step Evaluate(Command::Context & context, Unit & unit) { return {}; }
	bool EvaluateEveryTick() { return false; }

	virtual ~Action_Command()
	{ }

	virtual Action_Command * clone() const
	{
		return new Action_Command{};
	}
};



} // namespace Brushlink

#endif // BRUSHLINK_COMMAND_H
