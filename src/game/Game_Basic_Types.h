#pragma once
#ifndef BRUSHLINK_GAME_BASIC_TYPES_H
#define BRUSHLINK_GAME_BASIC_TYPES_H

#include <random>
#include <vector>

#include "BuiltinTypedefs.h"
#include "NamedType.hpp"

namespace Brushlink
{

using Farb::HString;
using Farb::NamedType;

struct PlayerIDTag
{
	static HString GetName() { return "PlayerID"; }
};
using PlayerID = NamedType<int, PlayerIDTag>;

struct UnitIDTag
{
	static HString GetName() { return "UnitID"; }
};
using UnitID = NamedType<int, UnitIDTag>;

struct Unit_Group
{
	std::vector<UnitID> members;
};

enum class Unit_Type
{
	Spawner,
	Healer,
	Attacker,
};

enum class Unit_Attribute
{
	Energy,
	Movement_Speed,
	Vision_Radius,
};

Unit_Type GetRandomUnitType(std::mt19937 generator);

} // namespace Brushlink

#endif // BRUSHLINK_GAME_BASIC_TYPES_H
