

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
			},
			6,
			{} // targeted_modifiers
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
			},
			4,
			{ // targeted_modifiers
				{ Action_Type.Heal, { {-1} } }
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
			},
			4,
			{} // targeted_modifiers
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
	// this behavior is currently dependent on the order of operations
	// we should consider storing pre-step, transition, and post-step states
	// such as units leaving/entering spaces, spending energy / getting healed
	// how to reconcile these deterministically?
	// could construct dependency graphs. some cycles might be okay, such as units swapping positions.
	// being attacked and therefore not having enough energy to take an action
	// vs taking the action and then dying. which is preferable? per-unit player setting?

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
			Action_Type.Heal,
		},
		{
			Action_Type.Reproduce,
		},
		{
			Action_Type.Attack,
		},
		{
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
			unit_count_at_last_loop = units_remaining.size();
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
						AddUnitToAct(unit);
						continue;
					}
				} // for units
			} // for action type
		} // while units_remaining
	} // for action_group
}

Action_Result Game::UnitTakeAction(Unit & unit)
{
	if (unit.pending.type == Action_Type.Nothing
		|| unit.pending.type == Action_Type.Idle)
	{
		// consider recompute here
		return Action_Result.Success;
	}
	if (!Contains(unit.type->actions, unit.pending.type))
	{
		return Action_Result.Recompute;
	}
	Action_Settings & action_settings = unit.type.actions[unit.pending.type];
	Ticks cooldown = SecondsToTicks(action_settings.cooldown);
	Ticks max_last_tick_same {-1 * cooldown.value};
	Ticks max_next_tick_any {-1};
	for (auto & pair : unit.history)
	{
		if (pair.first == unit.pending.type
			&& max_last_tick_same < pair.second)
		{
			max_last_tick_same = pair.second;
		}
		int next_tick_any = pair.second.value
			+ SecondsToTicks(
				unit.type.actions[pair.first].duration
			).value;
		if (next_tick_any > max_next_tick_any.value)
		{
			max_next_tick_any.value = next_tick_any;
		}
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

	if (action_settings.cost > unit.energy)
	{
		// should there be a distinct result for not enough energy?
		// or should we just wait until we can?
		return Action_Result.Waiting;
	}

	Energy magnitude = action_settings.magnitude;
	if (unit.pending.target)
	{
		Unit * target = world.GetUnit(unit.pending.target.value);
		if (target == nullptr)
		{
			return Action_Result.Recompute;
		}
		if (!unit.position.IsNeighbor(target->position))
		{
			return Action_Result.Recompute;
		}

		if (Contains(target->type->targeted_modifiers, unit.pending.type))
		{
			magnitude.value += target->type->targeted_modifiers[unit.pending.type].amount;
		}
	}

	// check for unique failures and take unique action steps
	switch(unit.pending.type)
	{
	case Action_Type.Attack:
		target->energy.value -= magnitude.value;
		break;
	case Action_Type.Heal:
		Unit * target = world.GetUnit(unit.pending.target.value);
		if (target->energy >= target->type->max_energy)
		{
			// consider Retry?
			return Action_Result.Recompute;
		}
		target->energy.value += magnitude.value;
		break;
	case Action_Type.Reproduce:
		Map<Unit_Type, int> neighbors;
		Map<Unit_Type, Point> type_positions;
		Unit_Type last_neighbor_type {-1};
		std::vector<Point> free_spaces;
		for (auto & pos : unit.position.GetNeighbors())
		{
			if (Contains(world.positions, pos))
			{
				Unit * neighbor = GetUnit(world.positions[pos]);
				neighbors[neighbor.type->type] += 1;
				last_neighbor_type = neighbor.type->type;
			}
			else
			{
				if (last_neighbor_type != Unit_Type{-1})
				{
					type_positions[last_neighbor_type] = pos;
				}
				free_spaces.push_back(pos);
			}
		}
		if (free_spaces.empty())
		{
			// consider Retry if in group with Move and unit is going to move;
			return Action_Result.Recompute;
		}
		Unit_Type most_common_type = GetRandomUnitType(random_generator);
		int max_neighbor_type_count = 0;
		for (auto & pair : neighbors)
		{
			if (pair.second > max_neighbor_type_count)
			{
				max_neighbor_type_count = pair.second;
				most_common_type = pair.first;
			}
		}
		Point position = free_spaces[0];
		if (Contains(type_positions, most_common_type))
		{
			position = type_positions[most_common_type];
		}

		auto result = SpawnUnit(unit.player, most_common_type, position);
		if (result.IsError())
		{
			return Action_Result.Recompute;
		}
		// todo: emit event for unit spawned
		break;
	case Action_Type.Move:
		if (!unit.position.IsCardinalNeighbor(unit.pending.location.value))
		{
			return Action_Result.Recompute;
		}
		bool success = world.MoveUnit(unit.id, unit.pending.location.value);
		if (!success)
		{
			// if we use Recompute here, chains of units moving in a line
			// could either all move together or one by one
			// alternatively use pre/post tick states to make this behavior consistent
			return Action_Result.Retry;
		}

		// @Feature special case here for swapping?
		// how to do so in context tracking which units have already updated
		break;
	}

	// assume success here, common items for taking the action
	unit.history[unit.pending.type] = tick;
	unit.energy -= action_settings.cost;

	return Action_Result.Success;
}


void Game::ApplyCrowdingDecayAndPruneExhaustedUnits()
{
	Ticks crowded_decay_time = tick - SecondsToTicks(settings.crowded_decay.second);
	Energy crowded_decay_amount = settings.crowded_decay.first;
	Set<UnitID> exhausted;
	for (auto & pair : world.units)
	{
		Unit & unit = pair.second;
		// Crowding
		int neighbor_count = 0;
		for (auto & pos : unit.position.GetNeighbors())
		{
			if (Contains(world.positions, pos))
			{
				neighbor_count += 1;
			}
		}
		unit.crowded_duration.value += neighbor_count >= settings.crowded_threshold.value
			? 1
			: -1;
		if (unit.crowded_duration.value >= crowded_decay_time.value)
		{
			unit.energy.value -= crowded_decay_amount.value;
			unit.crowded_duration.value -= crowded_decay_time.value;
		}
		if (unit.crowded_duration.value < 0)
		{
			unit.crowded_duration.value = 0;
		}

		// Exhaustion
		if (unit.energy.value < 0)
		{
			exhausted.insert(unit.id);
		}

		// Energy cap
		if (unit.energy.value > unit.type->max_energy.value)
		{
			unit.energy = unit.type->max_energy;
		}
	}
	RemoveUnits(exhausted);
}


ErrorOr<UnitID> Game::SpawnUnit(PlayerID player, Unit_Type type, Point position)
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

void Game::RemoveUnits(Set<UnitID> units)
{
	crowded_history
	world.RemoveUnits(units);
	for (auto & pair : players)
	{
		pair.second.RemoveUnits(units);
	}
	for (auto u_id : units)
	{
		crowded_history.remove(u_id);
	}
}

Ticks Game::SecondsToTicks(Seconds s)
{
	return Ticks{ Round(seconds.value * settings.speed.value) };
}