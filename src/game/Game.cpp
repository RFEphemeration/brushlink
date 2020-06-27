
#include "Game.h"

#include <bitset>
#include <cstdio>

#include "tigr.h"

#include "IntExtensions.hpp"

namespace Brushlink
{

const GameSettings GameSettings::default_settings {
	Map<PlayerID, Player_Settings> {
		{{0}, {
			Player_Type::Local_Player}
		},
		{{1}, {
			Player_Type::AI}
		},
	},
	Map<Unit_Type, Unit_Settings> {
		{ Unit_Type::Spawner, {
			Unit_Type::Spawner,
			{6}, {24}, {{1}, {0.5}}, // more energy in order to reduce healing and make harder to kill
			{}, // todo: drawn_body
			{
				{ Action_Type::Nothing, Action_Settings{} },
				{ Action_Type::Idle, Action_Settings{} },
				{ Action_Type::Move, Action_Settings{{0}, {0}, {0.5}, {0.25}} },
				{ Action_Type::Reproduce, Action_Settings{{15}, {0}, {4.0}, {1.0}} },
			},
			6.6,
			{} // targeted_modifiers
		}},
		{ Unit_Type::Healer, {
			Unit_Type::Healer,
			{8}, {12}, {{1}, {1.0}}, // starts at moderate health, mild regen
			{}, // todo: drawn_body
			{
				{ Action_Type::Nothing, Action_Settings{} },
				{ Action_Type::Idle, Action_Settings{} },
				{ Action_Type::Move, Action_Settings{{0}, {0}, {2.0 / 3.0}, {1.0 / 3.0}} },
				{ Action_Type::Heal, Action_Settings{{2}, {3}, {1.5}, {1.0 / 6.0}} },
				// todo: heal action target modifier so healing a healer is 1-1
			},
			4.6,
			{ // targeted_modifiers
				{ Action_Type::Heal, { {-1} } }
			}

		}},
		{ Unit_Type::Attacker, {
			Unit_Type::Attacker,
			{12}, {12}, {{1}, {6.0}}, // starts at full health, very slow regen
			{}, // todo: drawn_body
			{
				{ Action_Type::Nothing, Action_Settings{} },
				{ Action_Type::Idle, Action_Settings{} },
				{ Action_Type::Move, Action_Settings{{0}, {0}, {1.0 / 3.0}, {1.0 / 6.0}} },
				{ Action_Type::Attack, Action_Settings{{0}, {3}, {1.0}, {1.0 / 6.0}} },
			},
			4.6,
			{} // targeted_modifiers
		}},
	},
	Map<Point, Unit_Type>{ // starting units
		{ {-1, -1}, Unit_Type::Spawner },
		{ {1, -1}, Unit_Type::Spawner },
		{ {-1, 1}, Unit_Type::Attacker },
		{ {1, 1}, Unit_Type::Healer },
	},
	"res/images/energy_bars.png",
	"res/images/units.png",
	"res/images/palettes.png",
	{ // palette_replace_colors
		{192, 192, 192, 255},
		{128, 128, 128, 255},
		{64, 64, 64, 64},
	},
	255 * 255 / 2, // palette_diff_squared_requirement 1/8 of max
	Ticks {12},
	Number {6},
	std::pair<Energy, Seconds>{{1}, {1.0}}
};

void Game::Initialize()
{
	if (world.settings.starting_locations.size() < settings.player_settings.size())
	{
		Error("Not enough starting locations").Log();
		// early quit, prevent starting the game, etc?
		return;
	}
	std::shared_ptr<Tigr> palettes_image{
		tigrLoadImage(settings.palettes_image_file.c_str()),
		TigrDeleter{}};
	for (auto & player_pair : settings.player_settings)
	{
		PlayerID id = player_pair.first;
		players[id] = Player::FromSettings(player_pair.second, id, world.settings.starting_locations[id.value]);
		for (auto & graphics : players[id].data->graphical_preferences)
		{
			if (graphics.palette.type == Palette_Type::SingleColor
				|| graphics.palette.type == Palette_Type::Custom)
			{
				continue;
			}
			for (int i = 0; i < palettes_image->w; i++)
			{
				graphics.palette.colors[i] = tigrGet(
					palettes_image.get(),
					i,
					static_cast<int>(graphics.palette.type)
				);
				printf("%08x\n", Pack(graphics.palette.colors[i]));
				//std::cout << std::bitset<32>{Pack(graphics.palette.colors[i])} << std::endl;
			}
		}
		players[id].graphics = players[id].data->graphical_preferences.front();
		if (players[id].settings.type == Player_Type::Local_Player
			|| local_player.value == -1)
		{
			local_player = id;
		}
	}
	world.energy_bars.reset(tigrLoadImage(settings.energy_image_file.c_str()));
	for (auto & [player_id, player] : players)
	{
		world.player_graphics[player_id] = player.graphics;

		for (auto & [offset, unit_type] : settings.starting_units)
		{
			auto result = SpawnUnit(
				player_id,
				unit_type,
				player.starting_location + offset);
			if (result.IsError())
			{
				// what now?
				result.GetError().Log();
			}
		}
	}
	std::shared_ptr<Tigr> units_image{tigrLoadImage(settings.units_image_file.c_str()), TigrDeleter{}};
	int px = world.settings.tile_px;
	int color_count = settings.palette_replace_colors.size();
	std::vector<uint> packed_colors;
	for(auto & color : settings.palette_replace_colors)
	{
		packed_colors.push_back(Pack(color));
	}
	for (auto & [unit_type, unit_settings] : settings.unit_types)
	{
		for (auto & [player_id, graphics] : world.player_graphics)
		{
			if (graphics.palette.color_count < color_count)
			{
				Error("Player Palette doesn't have enough colors").Log();
				continue;
			}
			std::shared_ptr<Tigr> body {tigrBitmap(px, px), TigrDeleter{}};
			tigrBlit(body.get(),
				units_image.get(),
				0, 0, // dest x,y
				px * static_cast<int>(unit_type),
				px * static_cast<int>(graphics.pattern),
				px, px // size
			);
			for (int x = 0; x < px; x++)
			{
				for (int y = 0; y < px; y++)
				{
					uint packed_color = Pack(body->pix[y*px+x]);
					for (int color_index = 0; color_index < color_count; color_index++)
					{
						if (packed_color == packed_colors[color_index])
						{
							body->pix[y*px+x] = graphics.palette.colors[color_index];
						}
					}
				}
			}
			unit_settings.drawn_body.insert_or_assign(graphics, body);
		}
	}
}

Input_Result Game::ReceiveInput(
	const Key_Changes &,
	const Modifiers_State &,
	const Mouse_State &,
	const std::vector<Point> &)
{
	return Input_Result::NoUpdateNeeded;
}

void Game::Tick()
{
	// should this be before or after update functions?
	tick.value += 1;
	ProcessPlayerInput();
	RunPlayerCoroutines();
	AllUnitsTakeAction();
	EnergyTick();
}

void Game::ProcessPlayerInput()
{

}

void Game::RunPlayerCoroutines()
{

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
		{Action_Type::Attack, {}},
		{Action_Type::Heal, {}},
		{Action_Type::Reproduce, {}},
		{Action_Type::Move, {}},
	};
	std::vector<std::vector<Action_Type>> grouped_action_order {
		{
			Action_Type::Heal,
		},
		{
			Action_Type::Reproduce,
		},
		{
			Action_Type::Attack,
		},
		{
			Action_Type::Move,
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
			units_remaining.erase(unit.id);
		}
	};

	auto UpdateUnitAction = [&](Unit & unit)
	{
		Command * command = unit.command_queue.empty()
				? unit.idle_command.get()
				: unit.command_queue.front().get();
		unit.pending = command->Evaluate(
			players[unit.player].root_command_context,
			unit);
		if (unit.pending.type == Action_Type::Idle && !unit.command_queue.empty())
		{
			unit.command_queue.pop();
		}
	};

	for (auto & pair :  world.units)
	{
		Unit & unit = pair.second;
		if (unit.pending.type == Action_Type::Idle)
		{
			UpdateUnitAction(unit);
		}
		else if (unit.pending.type == Action_Type::Nothing
			&& !unit.command_queue.empty()
			// is EvaluateEveryTick for coroutines the same as just updating idle action?
			&& unit.command_queue.front()->EvaluateEveryTick())
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
					case Action_Result::Waiting:
						units_remaining.erase(unit.id);
						continue;
					case Action_Result::Success:
						units_remaining.erase(unit.id);
						// each action is only done a single time
						// and repeat actions need to be handled by commands
						unit.pending.type = Action_Type::Idle;
						continue;
					case Action_Result::Retry:
						AddUnitToAct(unit);
						continue;
					case Action_Result::Recompute:
						// should we no longer evaluate this frame?
						// otherwise you could potentially get stuck
						// adding the same unit over and over again
						// I think it's okay so long as remaining units goes down
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
	if (unit.pending.type == Action_Type::Nothing
		|| unit.pending.type == Action_Type::Idle)
	{
		// consider recompute here
		return Action_Result::Success;
	}
	if (!Contains(unit.type->actions, unit.pending.type))
	{
		return Action_Result::Recompute;
	}
	Action_Settings & action_settings = unit.type->actions[unit.pending.type];
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
				unit.type->actions[pair.first].duration
			).value;
		if (next_tick_any > max_next_tick_any.value)
		{
			max_next_tick_any.value = next_tick_any;
		}
	}

	Ticks next_tick_same { max_last_tick_same.value
		+ SecondsToTicks(
			unit.type->actions[unit.pending.type].cooldown
		).value
	};

	if (max_next_tick_any.value > tick.value
		|| next_tick_same.value > tick.value)
	{
		return Action_Result::Waiting;
	}

	if (action_settings.cost > unit.energy)
	{
		// should there be a distinct result for not enough energy?
		// or should we just wait until we can?
		return Action_Result::Waiting;
	}

	Energy magnitude = action_settings.magnitude;
	Unit * target = nullptr;
	if (unit.pending.target)
	{
		target = world.GetUnit(unit.pending.target.value());
		if (target == nullptr)
		{
			return Action_Result::Recompute;
		}
		if (!unit.position.IsNeighbor(target->position))
		{
			return Action_Result::Recompute;
		}

		if (Contains(target->type->targeted_modifiers, unit.pending.type))
		{
			magnitude.value += target->type->targeted_modifiers[unit.pending.type].amount.value;
		}
	}

	// check for unique failures and take unique action steps
	switch(unit.pending.type)
	{
	case Action_Type::Nothing:
	case Action_Type::Idle:
		return Action_Result::Recompute;
	case Action_Type::Attack:
		target->energy.value -= magnitude.value;
		break;
	case Action_Type::Heal:
		if (target->energy.value >= target->type->max_energy.value)
		{
			// consider Retry?
			return Action_Result::Recompute;
		}
		target->energy.value += magnitude.value;
		break;
	case Action_Type::Reproduce:
	{
		Map<Unit_Type, int> neighbors;
		Map<Unit_Type, Point> type_positions;
		Unit_Type last_neighbor_type {-1};
		std::vector<Point> free_spaces;
		for (auto & pos : unit.position.GetNeighbors())
		{
			if (Contains(world.positions, pos))
			{
				Unit * neighbor = world.GetUnit(world.positions[pos]);
				neighbors[neighbor->type->type] += 1;
				last_neighbor_type = neighbor->type->type;
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
			return Action_Result::Recompute;
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
			return Action_Result::Recompute;
		}
		// todo: emit event for unit spawned
		break;
	}
	case Action_Type::Move:
		if (!unit.position.IsCardinalNeighbor(unit.pending.location.value()))
		{
			return Action_Result::Recompute;
		}
		bool success = world.MoveUnit(unit.id, unit.pending.location.value());
		if (!success)
		{
			// if we use Recompute here, chains of units moving in a line
			// could either all move together or one by one
			// alternatively use pre/post tick states to make this behavior consistent
			return Action_Result::Retry;
		}

		// @Feature special case here for swapping?
		// how to do so in context tracking which units have already updated
		break;
	}

	// assume success here, common items for taking the action
	unit.history[unit.pending.type] = tick;
	unit.energy.value -= action_settings.cost.value;

	return Action_Result::Success;
}


void Game::EnergyTick()
{
	Ticks crowded_decay_time {SecondsToTicks(settings.crowded_decay.second).value};
	Energy crowded_decay_amount = settings.crowded_decay.first;
	Set<UnitID> exhausted;
	for (auto & pair : world.units)
	{
		Unit & unit = pair.second;
		// Energy recharge
		Ticks recharge_frequency = SecondsToTicks(unit.type->recharge_rate.second);
		Energy recharge_amount = unit.type->recharge_rate.first;
		// @Feature SpawnTime in order to offset/stagger recharges?
		if (tick.value % recharge_frequency.value == 0)
		{
			unit.energy.value += recharge_amount.value;
		}

		// Energy cap
		if (unit.energy.value > unit.type->max_energy.value)
		{
			unit.energy = unit.type->max_energy;
		}

		// Crowding
		int neighbor_count = 0;
		for (auto & pos : unit.position.GetNeighbors())
		{
			if (Contains(world.positions, pos))
			{
				neighbor_count += 1;
			}
		}
		if (neighbor_count >= settings.crowded_threshold.value)
		{
			unit.crowded_duration.value++;
			if (unit.crowded_duration.value >= crowded_decay_time.value)
			{
				unit.energy.value -= crowded_decay_amount.value;
				unit.crowded_duration.value -= crowded_decay_time.value;
			}
		}
		else
		{
			unit.crowded_duration.value--;
			if (unit.crowded_duration.value < 0)
			{
				unit.crowded_duration.value = 0;
			}
		}

		// Exhaustion
		if (unit.energy.value < 0)
		{
			exhausted.insert(unit.id);
		}
	}
	RemoveUnits(exhausted);
}

void Game::Render(Tigr * screen, const Dimensions & world_portion)
{
	world.Render(screen, world_portion, players[local_player].camera_location, local_player);
	// todo: render command card, buffer
}

bool Game::IsOver()
{
	Set<PlayerID> players_with_units;
	for(auto & pair : world.units)
	{
		players_with_units.insert(pair.second.player);
		if (players_with_units.size() > 1)
		{
			return false;
		}
	}
	return true;
}


ErrorOr<UnitID> Game::SpawnUnit(PlayerID player, Unit_Type type, Point position)
{
	Unit u;
	u.type = &(settings.unit_types[type]);
	u.id = next_unit_id;
	u.player = player;
	u.position = position;
	u.energy = u.type->starting_energy;
	// todo: idle command, pending action

	bool success = world.AddUnit(std::move(u), position);
	if (!success)
	{
		return Error("Couldn't add unit to world");
	}
	UnitID id = next_unit_id;
	next_unit_id.value++;
	return id;
}

void Game::RemoveUnits(Set<UnitID> units)
{
	world.RemoveUnits(units);
	for (auto & pair : players)
	{
		pair.second.RemoveUnits(units);
	}
}

Ticks Game::SecondsToTicks(Seconds s)
{
	return Ticks{ Round(s.value * settings.speed.value) };
}

} // namespace Brushlink
