#pragma once
#ifndef BRUSHLINK_UNIT_BASICS_H
#define BRUSHLINK_UNIT_BASICS_H

#include <random>
#include <vector>

namespace Brushlink
{

struct UnitIDTag
{
	static HString GetName() { return "UnitID"; }
};
using UnitID = NamedType<int, UnitIDTag>;

struct UnitGroup
{
	std::vector<UnitID> members;
};

enum class Unit_Type
{
	Spawner,
	Healer,
	Attacker,
};

Unit_Type GetRandomUnitType(std::mt19937 generator);

} // namespace Brushlink

#endif // BRUSHLINK_UNIT_BASICS_H
