void Move(CommandContext & context, Unit_Group actors, Point location)
{
	for (auto & unit_id : actors.members)
	{
		Unit & unit = context.GetUnit(unit_id);
		unit.command_queue.clear();
		unit.command_queue.push_back()
	}
}

struct Action_Move : Action_Command
{
	Point location;
	Action_Step Evaluate(CommandContext & context, Unit & unit)
	{
		Point next = unit.position;
		int distance = location.CardinalDistance(next);
		if (distance == 0)
		{
			return {idle, nullptr, nullptr};
		}
		for (auto & neighbor : GetCardinalNeighbors(unit.position))
		{
			// should we check if it's empty here?
			int neighbor_distance = location.CardinalDistance(neighbor.position);
			if (neighbor_distance < distance)
			{
				next = neighbor;
				distance = neighbor_distance;
			}
			else if (neighbor_distance == distance
				&& Contains(context.game.world.positions, next)
				&& !Contains(context.game.world.positions, neighbor))
			{
				next = neighbor;
			}
		}
	}
}

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

// how to apply type restrictions to which ElementNames are accepted?
void SetUnitCommand(UnitId unit_id, ElementName command, std::vector<Value> parameters)
{
	Unit & unit = context.GetUnit(unit_id);

}