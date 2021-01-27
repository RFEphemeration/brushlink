#pragma once
#ifndef BRUSHLINK_UNIT_H
#define BRUSHLINK_UNIT_H

#include <vector>
#include <queue>
#include <utility>

#include "BuiltinTypedefs.h"

#include "Resources.h"
#include "Player_Graphics.h"
#include "Game_Basic_Types.h"
#include "Command.h"
#include "Action.h"
#include "Player.h"

namespace Brushlink
{

struct Unit_Settings
{
	Unit_Type type;
	Energy starting_energy {6};
	Energy max_energy {12};
	std::pair<Energy, Seconds> recharge_rate {{1}, {1.0}};
	Map<Player_Graphics, std::shared_ptr<Tigr>> drawn_body;
	Map<Action_Type, Action_Settings> actions;
	float vision_radius = 4.5;
	Map<Action_Type, Action_Magnitude_Modifier> targeted_modifiers;
};

struct Unit
{
	Unit_Settings * type;
	UnitID id;
	PlayerID player;
	Point position;
	Energy energy;
	Ticks crowded_duration;

	Action_Step pending; // if pending.type == Idle there is no pending
	Map<Action_Type, Ticks> history;
	// Command type in unit context?
	// need an already executed type stored by value
	// and a repeatedly executed type full tree
	std::queue<value_ptr<Action_Command> > command_queue;
	value_ptr<Action_Command> idle_command;
};



} // namespace Brushlink

#endif // BRUSHLINK_UNIT_H
