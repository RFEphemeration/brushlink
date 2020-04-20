#ifndef BRUSHLINK_COMMAND_VALUE_TYPES_H
#define BRUSHLINK_COMMAND_VALUE_TYPES_H

#include <vector>
#include <functional>
#include <variant>

#include "BuiltinTypedefs.h"
#include "ErrorOr.hpp"
#include "NamedType.hpp"
#include "Math.hpp"

using namespace Farb;

namespace Command
{

using Command = Success;

// these might need to be Ref<>s or ptrs
// using Action = Functor<void, CommandContext>;

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
};

// not actually used in command statements
struct UnitIDTag
{
	static HString GetName() { return "Command::UnitID"; }
};
using UnitID = NamedType<int, UnitIDTag>;

struct UnitGroup
{
	std::vector<UnitID> members;
};

struct NumberTag
{
	static HString GetName() { return "Command::Number"; }
};
using Number = NamedType<int, NumberTag>;

using Filter = std::function<UnitGroup(UnitGroup)>;
// is GroupSize this just a number?
using GroupSize = std::function<Number(UnitGroup)>;
using Superlative = std::function<UnitGroup(Number, UnitGroup)>;

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
};

enum class Ability_Type
{
	Move,
	Damage,
	Single_Target,
	AOE,
	Heal,
	Building,
	Terrain
};

enum class Attribute_Type
{
	Health,
	Energy,
	Damage,
	DPS,
	Movement_Speed
};

struct Point
{
	int x;
	int y;

	Point operator + (const Point & other)
	{
		return Point(x + other.x, y + other.y);
	}

	Point operator / (const int divisor)
	{
		return Point(x / divisor, y / divisor);
	}
};

struct Line
{
	std::vector<Point> points;
};

struct Direction
{
	int x;
	int y;

	Direction(Point from, Point to)
		: x(to.x - from.x)
		, y(to.y - from.y)
	{ }
};

enum class PointDistributionMethod
{
	Fibonacci,
	Grid,
	Random
};

struct Area_Interface
{
	virtual std::vector<Point> GetPointDistributionInArea(Number count) const;

	virtual bool Contains(Point point) const = 0;

	virtual Point GetCenter() const = 0;

	// this currently returns a diameter, but maybe it should be a radius
	virtual double GetLargestDimension() const = 0;

	virtual double GetArea() const = 0;
};

struct Box : Area_Interface
{
	Point topLeft;
	Point bottomRight;

	bool Contains(Point point) const;

	Point GetCenter() const;

	double GetLargestDimension() const;

	double GetArea() const;
};

struct Circle : Area_Interface
{
	Point center;
	Number radius;

	bool Contains(Point point) const;

	Point GetCenter() const;

	double GetLargestDimension() const;

	double GetArea() const;
};

struct Perimeter : Area_Interface
{
	Line perimeter;

	bool Contains(Point point) const;

	Point GetCenter() const;

	double GetLargestDimension() const;

	double GetArea() const;
};

struct Area_Union : Area_Interface
{
	// can these be disjoint?
	std::vector<std::unique_ptr<Area_Interface>> areas;

	bool Contains(Point point) const;

	Point GetCenter() const;

	double GetLargestDimension() const;

	double GetArea() const;
};

struct Area_Intersection : Area_Interface
{
	// what happens if these are disjoint?
	std::vector<std::unique_ptr<Area_Interface>> areas;

	bool Contains(Point point) const;

	Point GetCenter() const;

	double GetLargestDimension() const;

	double GetArea() const;
};

// should this be a union type?
struct Area
{
	std::variant<Box, Circle, Perimeter, Area_Union, Area_Intersection> implementation;

	Area_Interface * interface = &implementation;

	std::vector<Point> GetPointDistributionInArea(Number count)
	{
		return implementation->GetPointDistributionInArea(count);
	}

	bool Contains(Point point)
	{
		return implementation->Contains(point);
	}
};

using Location = std::variant<Point, Line, Direction, Area>;

using Value = std::variant<
	Command,
	Action_Type,
	UnitGroup,
	Number,
	Selector_Filter,
	Selector_GroupSize,
	Selector_Superlative,
	Unit_Type,
	Ability_Type,
	Attribute_Type,
	Location,
	Point,
	Line,
	Direction,
	Area>;

// relying on the declaration parameter type checking
// to verify that there is at least one for repeatable
// but not optional parameters
template<typename T>
using Repeatable = std::vector<T>;

template<typename T>
using OptionalRepeatable = std::vector<T>;

// this optional should only be used for things that have
// no default value, since if there is a default
// we just pass that to the function instead
template<typename T>
using Optional = std::optional<T>;

template<typename Ts...>
using One_Of = std::variant<Ts...>;


template<typename TValue>
struct Underlying
{
	using Type = TValue;
};

template<typename TValue>
struct Underlying<Repeatable<TValue> >
{
	using Type = TValue;
};

template<typename TValue>
struct Underlying<Optional<TValue> >
{
	using Type = TValue;
};

template<typename TValue>
struct Underlying<OptionalRepeatable<TValue> >
{
	using Type = TValue;
};

} // namespace Command

#endif // BRUSHLINK_COMMAND_VALUE_TYPES_H

