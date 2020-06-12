

const GameSettings GameSettings::default_settings {
	Map<PlayerID, PlayerSettings> {
		{{0}, {
			Player_Type.Local_Player}
		},
		{{1}, {
			Player_Type.AI}
		},
	},
	Map<Unit_Type, UnitSettings> {
		{ Unit_Type.Spawner, {
			Unit_Type.Spawner,
			{6}, {24}, {{1}, {0.5}}, // more energy in order to reduce healing and make harder to kill
			{}, // todo: drawn_body
			{
				{ Action_Type.Nothing, Action_Settings{} },
				{ Action_Type.Idle, Action_Settings{} },
				{ Action_Type.Move, Action_Settings{{0}, {0}, {0.5}, {0.25}} },
				{ Action_Type.Reproduce, Action_Settings{{15}, {0}, {4.0}, {1.0}} },
			}
		}},
		{ Unit_Type.Healer, {
			Unit_Type.Healer,
			{8}, {12}, {{1}, {1.0}}, // starts at moderate health, mild regen
			{}, // todo: drawn_body
			{
				{ Action_Type.Nothing, Action_Settings{} },
				{ Action_Type.Idle, Action_Settings{} },
				{ Action_Type.Move, Action_Settings{{0}, {0}, {2.0 / 3.0}, {1.0 / 3.0}} },
				{ Action_Type.Heal, Action_Settings{{2}, {3}, {1.5}, {1.0 / 6.0}} },
				// todo: heal action target modifier so healing a healer is 1-1
			}
		}},
		{ Unit_Type.Attacker, {
			Unit_Type.Attacker,
			{12}, {12}, {{1}, {6.0}}, // starts at full health, very slow regen
			{}, // todo: drawn_body
			{
				{ Action_Type.Nothing, Action_Settings{} },
				{ Action_Type.Idle, Action_Settings{} },
				{ Action_Type.Move, Action_Settings{{0}, {0}, {1.0 / 3.0}, {1.0 / 6.0}} },
				{ Action_Type.Attack, Action_Settings{{0}, {3}, {1.0}, {1.0 / 6.0}} },
			}
		}},
	},
	Ticks {12},
};

void Game::Tick()
{
	tick += 1;
	ProcessPlayerInput();
	RunPlayerCoroutines();
	AllUnitsTakeAction();
	PruneExhaustedUnits();
}

void Game::AllUnitsTakeAction()
{
	// if we're adding units with reproducing, then maybe world.units will change
	// so we should consider not storing these Unit * and just call get every time
	// but apparently std::map and std::unordered_map both have stable memory
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
					Action_Result result = UnitTakeAction(unit);
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

Action_Result Game::UnitTakeAction(Unit & unit)
{
	if (unit.pending.type == Action_Type.Nothing
		|| unit.pending.type == Action_Type.Idle)
	{
		return;
	}
	Action_Settings & settings = unit.type.actions[unit.pending.type];
	Ticks cooldown = SecondsToTicks(settings.cooldown);
	Ticks max_last_tick_same {-1 * cooldown.value};
	Ticks max_next_tick_any {-1};
	for (auto & pair : unit.history)
	{
		if (pair.first == unit.pending.type
			&& max_last_tick_same < pair.second)
		{
			max_last_tick_same = pair.second;
		}
		max_next_tick_any.value = pair.second.value
			+ SecondsToTicks(
				unit.type.actions[pair.first].duration
			).value;
	}

	Ticks next_tick_same { max_last_tick_same.value
		+ SecondsToTicks(
			unit.type.actions[unit.pending.type].cooldown
		).value
	};

	if (max_next_tick_any.value > tick.value
		|| next_tick_same.value > tick.value)
	{
		return Action_Result.Waiting;
	}

	if (settings.cost > unit.energy)
	{
		// should there be a distinct result for not enough energy?
		// or should we just wait until we can?
		return Action_Result.Waiting;
	}
	switch(unit.pending.type)
	{
	case Action_Type.Attack:
		Unit * target = world.GetUnit(unit.pending.target);
		if (target == nullptr)
		{
			return Action_Result.Recompute;
		}
		if (!unit.position.IsNeighbor(target->position))
		{
			return Action_Result.Recompute;
		}

	case Action_Type.Heal:

	case Action_Type.Reproduce:

	case Action_Type.Move:
		if (!unit.position.IsCardinalNeighbor(unit.pending.location))
		{
			return Action_Result.Recompute;
		}

	}

	unit.history[unit.pending.type] = tick;
	return Action_Result.Success;
}


void Game::PruneExhaustedUnits()
{
	Set<UnitID> to_prune;
	for (auto & pair : world.units)
	{
		Unit & unit = pair.second;
		if (unit.energy.value < 0)
		{
			to_prune.insert(unit.id);
		}
	}
	world.RemoveUnits(to_prune);
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

Ticks Game::SecondsToTicks(Seconds s)
{
	return Ticks{ Round(seconds.value * settings.speed.value) };
}