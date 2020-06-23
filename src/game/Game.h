#pragma once
#ifndef BRUSHLINK_GAME_H
#define BRUSHLINK_GAME_H

#include <random>
#include <utility>

#include "BuiltinTypedefs.h"

#include "Game_Basic_Types.h"
#include "Player.h"
#include "World.h"

namespace Brushlink
{

struct GameSettings
{
	Map<PlayerID, Player_Settings> player_settings;
	Map<Unit_Type, Unit_Settings> unit_types;
	// offset from starting_position
	Map<Point, Unit_Type> starting_units;
	std::string energy_image_file;
	std::string units_image_file;
	std::string palettes_image_file;
	std::vector<TPixel> palette_replace_colors;
	int palette_diff_squared_requirement;
	Ticks speed {12}; // per second
	Number crowded_threshold {6}; // number of neighbors at which we start decaying
	std::pair<Energy, Seconds> crowded_decay{{1}, {1.0}};

	static const GameSettings default_settings;
};

struct Game
{
	GameSettings settings;
	World world;
	Map<PlayerID, Player> players;

	std::random_device random_seed; // seed
	std::mt19937 random_generator; // mersene twister from seed

	UnitID next_unit_id;
	Ticks tick;

	Game(const GameSettings & settings = GameSettings::default_settings)
		: settings(settings)
	{ }

	void Initialize();

	void Tick();
	// helper functions for Tick
	void ProcessPlayerInput();
	void RunPlayerCoroutines();
	void AllUnitsTakeAction();
	Action_Result UnitTakeAction(Unit & unit);
	void EnergyTick();

	void Render(Tigr * screen, const Dimensions & world_portion);
	bool IsOver();

	ErrorOr<UnitID> SpawnUnit(PlayerID player, Unit_Type unit, Point position);
	void RemoveUnits(Set<UnitID> units);

	Ticks SecondsToTicks(Seconds s);
	
};

} // namespace Brushlink

#endif // BRUSHLINK_GAME_H
