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
	virtual std::vector<Point> GetPointsInArea(Number count) const;
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

} // namespace Command

#endif // BRUSHLINK_COMMAND_VALUE_TYPES_H

