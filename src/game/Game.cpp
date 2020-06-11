

void Game::Tick()
{
	tick += 1;

	Map<Action_Type, std::vector<Unit *>> units_to_act {
		{Action_Type.Attack, {}},
		{Action_Type.Heal, {}},
		{Action_Type.Reproduce, {}},
		{Action_Type.Move, {}},
	};
	std::vector<std::vector<Action_Type>> grouped_action_order {
		{
			Action_Type.Attack
		},
		{
			Action_Type.Heal,
			Action_Type.Reproduce,
			Action_Type.Move,
		}
	};
	Set<UnitID> units_remaining{};

	auto AddUnitToAct = [&](Unit & unit)
	{
		if (units_to_act.count(unit.pending.type) > 0)
		{
			units_to_act[unit.pending.type].push_back(&unit);
			units_remaining.insert(unit.id);
		}
		else
		{
			units_remaining.remove(unit.id);
		}
	};

	auto UpdateUnitAction = [&](Unit & unit)
	{
		Command * command = command_queue.empty()
				? &idle_command
				: command_queue.front();
		CommandEvaluation result = command->Evaluate(
			players[unit.player].GetEvaluationContext(),
			unit);
		unit.pending = result.action_event;
		if (result.finished && !command_queue.empty())
		{
			command_queue.pop_front();
		}
	};

	enum class Action_Result
	{
		Waiting,
		Success,
		Retry,
		Recompute
	};
	auto TakeAction = [&](Unit & unit) -> bool
	{

	};

	for (auto & pair :  world.units)
	{
		Unit & unit = pair.second;
		if (unit.pending.type == Action_Type.Idle)
		{
			UpdateUnitAction(unit);
		}
		else if (unit.pending.type == Action_Type.Nothing
			&& !command_queue.empty()
			// is EvaluateEveryTick for coroutines the same as just updating idle action?
			&& command_queue.front()->EvaluateEveryTick())
		{
			UpdateUnitAction(unit);
		}

		AddUnitToAct(unit);
	}

	int unit_count_at_last_loop = units_remaining.size() + 1;

	for (auto & action_group : grouped_action_order)
	{
		while (units_remaining.size() > 0
			&& units_remaining.size() < unit_count_at_last_loop)
		{
			for (auto action_type : action_group)
			{
				std::vector<Unit *> taking_action{std::move(units_to_act[action_type])};

				for (auto * p_unit : taking_action)
				{
					Unit & unit = *p_unit;
					Action_Result result = TakeAction(unit);
					switch(result)
					{
					case Action_Result.Waiting:
						units_remaining.remove(unit.id);
						continue;
					case Action_Result.Success:
						units_remaining.remove(unit.id);
						// each action is only done a single time
						// and repeat actions need to be handled by commands
						unit.pending.type = Action_Type.Idle;
						continue;
					case Action_Result.Retry:
						AddUnitToAct(unit);
						continue;
					case Action_Result.Recompute:
						// should we no longer evaluate this frame?
						// otherwise you could potentially get stuck
						// adding the same unit over and over again
						// I think it's okay so long as remaining units goes down
						auto previous_type = unit.pending.type;
						UpdateUnitAction(unit);
						if (previous_type == unit.pending.type
							|| unit.pending.type != Action_Type.Attack)
						{
							// we special case prevent attacking after other actions have been taken
							// @Cleanup if we ever have different groups need to generalize
							AddUnitToAct(unit);
						}
						else
						{
							units_remaining.remove(unit.id);
						}
						continue;
					}
				} // for units
			} // for action type
			unit_count_at_last_loop = units_remaining.size();
		} // while units_remaining
	} // for action_group

}

bool Game::SpawnUnit(PlayerID player, Unit_Type type, Point position)
{
	Unit u;
	u.type = &(settings.unit_types[type]);
	u.id = next_unit_id;
	u.player = player;
	u.position = position;
	u.energy = u.type->starting_energy;

	bool success = world.AddUnit(u, position);
	if (!success)
	{
		return Error("Couldn't add unit to world");
	}

	next_unit_id.value++;
	return u.id;
}