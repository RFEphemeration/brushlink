#pragma once
#ifndef BRUSHLINK_PLAYER_H
#define BRUSHLINK_PLAYER_H

#include <vector>

#include "NamedType.hpp"

#include "ElementName.h"
#include "Dictionary.h"
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

	Map<Unit_Type, Command::ElementName> starting_unit_idle_commands;
	Map<ValueName, Command::ElementName> starting_commands;

	std::vector<std::string> element_definition_folders;
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
	Command::Dictionary exposed_elements;
	Command::Dictionary hidden_elements;

	// these are set when the game starts, are map/matchup dependent
	PlayerID id;
	Player_Graphics graphics;
	Point starting_location;

	// these change over the course of the game
	Point camera_location;

	Command::Context root_command_context;
	Table<Number, UnitGroup> command_groups;

	// todo: command buffer, stored values, evaluation context, etc

	static Player FromSettings(const Player_Settings & settings, PlayerID id, Point starting_location);

	void RemoveUnits(Set<UnitID> unit_ids);

	ErrorOr<ElementToken> GetTokenForName(ElementName name);


};


} // namespace Brushlink

#endif // BRUSHLINK_PLAYER_H

