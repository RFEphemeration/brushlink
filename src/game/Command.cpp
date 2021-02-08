#include "Command.h"
#include "Context.h"
#include "Game.h"

using namespace Command;

namespace Brushlink
{

struct Action_Move : Action_Command
{
	Point location;

	Action_Move (Point location)
		: location(location)
	{ }

	Action_Step Evaluate(Command::Context & context, Unit & unit)
	{
		Point next = unit.position;
		int distance = location.CardinalDistance(next);
		if (distance == 0)
		{
			return {Action_Type::Idle, {}, {}};
		}
		for (auto & neighbor : unit.position.GetCardinalNeighbors())
		{
			// should we check if it's empty here?
			int neighbor_distance = location.CardinalDistance(neighbor);
			if (neighbor_distance < distance)
			{
				next = neighbor;
				distance = neighbor_distance;
			}
			else if (neighbor_distance == distance
				&& Contains(context.game->world.positions, next)
				&& !Contains(context.game->world.positions, neighbor))
			{
				next = neighbor;
			}
		}
		return {Action_Type::Move, {next}, {}};
	}
	
	Action_Command * clone() const override
	{
		return new Action_Move{*this};
	}
};

void Move(Command::Context & context, Unit_Group actors, Point location)
{
	for (auto & unit_id : actors.members)
	{
		ErrorOr<Ref<Unit>> result = context.GetUnit(unit_id);
		if (result.IsError())
		{
			// rmf todo: log invalid unit id? report back to user?
			continue;
		}
		auto unit = result.GetValue().get();
		std::queue<value_ptr<Action_Command> > empty;
		std::swap(unit.command_queue, empty);
		// rmf todo: get offset from average location
		unit.command_queue.push({new Action_Move{location}});
	}
}

/*
Action_Step MoveToward(Point destination)
{
	If PositionOf unit Equals destination
		Action_Step Idle
		Action_Step
			Move
			Nearest
				CardinalNeighbors Position unit
				destination
}

void MoveGroupTo(Point group_destination)
{
	Set group_position PositionOf Actors
	ForEach Actors unit
		Set destination
			Plus
				group_destination
				group_position Minus PositionOf unit
		// set vs append, is that tracked context now?
		SetUnitCommand unit
			MoveToward
			destination
			// optional statement of any type here to run in new context? for setting variables and such
}

Action_Step MultipleTickCommands()
{
	FirstWhere tick_commands command
		Sequence
			Set step command // evaluates command
			None // where
				Equal Nothing StepType step
				Equal Idle StepType step
		// how to break if not a per-tick command?
}
*/

/*
// how to apply type restrictions to which ElementNames are accepted?
void SetUnitCommand(UnitID unit_id, ElementName command, std::vector<Variant> parameters)
{
	Unit & unit = context.GetUnit(unit_id);

}
*/


} // namespace Brushlink
