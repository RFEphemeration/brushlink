#ifndef BRUSHLINK_COMMAND_VALUE_TYPES_H
#define BRUSHLINK_COMMAND_VALUE_TYPES_H

namespace Command
{

// not actually used in command statements, but given to units
enum class Action_Type
{
	Move_To,
	Move_Along,
	Attack,
	Cast,
	Follow,
	Repeat,
	Stop // or is this just not taking any other actions?
}

// not actually used in command statements
using UnitID = NamedType<int, NAMED_TYPE_TAG(UnitID)>;

struct UnitGroup
{
	std::vector<UnitID> members;
}

struct NumberTag
{
	static HString GetName() { return "Command::Number"; }
}
using Number = NamedType<int, Number>;

using Selector_Filter = Functor<UnitGroup, CommandContext, UnitGroup>;
using Selector_GroupSize = Functor<Number, CommandContext, UnitGroup>; // is this just a number?
using Selector_Superlative = Functor<UnitGroup, CommandContext, Number, UnitGroup>;

// each unit can have multiple types
enum class Unit_Type
{
	Worker,
	Transport,
	Scout,
	Spawner,
	Melee,
	Range,
	Healer,
	Caster,
	Ground,
	Air,
	Support,
	Tank,
	DPS
}

enum class Ability_Type
{
	Move,
	Damage,
	Single_Target,
	AOE,
	Heal,
	Building,
	Terrain
}

enum class Attribute_Type
{
	Health,
	Energy,
	Damage,
	DPS,
	Movement_Speed
}

struct Point
{
	int x;
	int y;
}

struct Line
{
	std::vector<Point> points;
}

struct Direction
{
	int x;
	int y;
}

struct Area
{
	virtual std::vector<Point> GetPointDistributionInArea(Number count) const;

	virtual bool IsPointInArea(Point point) const;
}

struct Box : Area
{
	Point topLeft;
	Point bottomRight;
}

struct Circle : Area
{
	Point center;
	Number radius;
}

struct Perimeter : Area
{
	Line perimeter;
}

template<typename T>
using OptionalRepeatable = std::vector<T>;

template<typename T>
using Optional = std::optional<T>;

} // namespace Command

#endif // BRUSHLINK_COMMAND_VALUE_TYPES_H

