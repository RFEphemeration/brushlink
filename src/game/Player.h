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
	std::vector<Player_Graphics> graphical_preferences;
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
};

// game time modified values
struct Player
{
	// loaded when created, don't change during course of game
	// though data may change in editor mode
	std::shared_ptr<Player_Data> data;
	Player_Settings settings;
	Player_Graphics graphics;

	// these change over the course of the game
	Point camera_location;
	CommandContext root_command_context;
	// todo: command buffer, stored values, evaluation context, etc

	void RemoveUnits(Set<UnitID> unit_ids);
};


} // namespace Brushlink

#endif // BRUSHLINK_PLAYER_H

