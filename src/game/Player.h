#pragma once
#ifndef BRUSHLINK_PLAYER_H
#define BRUSHLINK_PLAYER_H

#include <vector>

#include "NamedType.hpp"

#include "Player_Graphics.h"
#include "Location.h"
#include "Game_Basic_Types.h"
#include "Command.h"

namespace Brushlink
{

// loaded, persistant between games
struct Player_Data
{
	std::vector<Player_Graphics> graphical_preferences{
		{Pattern::Stripes, Palette{{}, 3, Palette_Type::Purple}},
		{Pattern::Checkers, Palette{{}, 3, Palette_Type::Green}},
		{Pattern::Spots, Palette{{}, 3, Palette_Type::Blue}},
		{Pattern::Grid, Palette{{}, 3, Palette_Type::Orange}},
	};

	Map<Unit_Type, value_ptr<Command>> starting_unit_idle_commands;
	Map<ValueName, value_ptr<Command>> starting_commands;
};

enum class Player_Type
{
	AI,
	Local_Player,
	// Todo?: Remote_Player
};

struct Player_Settings
{
	Player_Type type;
	// Todo: dictionary of command elements - should probably be in Player_Data
	// how to get Player_Data should probably be here
	// todo: make this load from a file
};

// game time modified values
struct Player
{
	// loaded when created, don't change during course of game
	// though data may change in editor mode
	std::shared_ptr<Player_Data> data;
	Player_Settings settings;

	// these are set when the game starts, are map/matchup dependent
	PlayerID id;
	Player_Graphics graphics;
	Point starting_location;

	// these change over the course of the game
	Point camera_location;
	CommandContext root_command_context;
	// todo: command buffer, stored values, evaluation context, etc

	static Player FromSettings(const Player_Settings & settings, PlayerID id, Point starting_location);

	void RemoveUnits(Set<UnitID> unit_ids);


};


} // namespace Brushlink

#endif // BRUSHLINK_PLAYER_H

