#ifndef BRUSHLINK_COMMAND_VALUE_TYPES_H
#define BRUSHLINK_COMMAND_VALUE_TYPES_H

#include <vector>
#include <functional>
#include <variant>

#include "BuiltinTypedefs.h"
#include "ErrorOr.hpp"
#include "NamedType.hpp"
#include "MathUtils.h"

#include "ElementType.h"
#include "Action.h"
#include "Location.h"
#include "Unit.h"

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

struct DigitTag
{
	static HString GetName() { return "Command::Digit"; }
};
using Digit = NamedType<int, DigitTag>;

using Filter = std::function<UnitGroup(UnitGroup)>;
// is GroupSize this just a number?
using GroupSize = std::function<Number(UnitGroup)>;
using Superlative = std::function<UnitGroup(UnitGroup, Number)>;

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

// int positions? fixed point float?
// int is probably granular enough for any closed map
using Point = Point2D<int>;

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
	virtual ~Area_Interface()
	{ }

	// this is intended to only be used by value_ptr internals
	virtual Area_Interface * clone() const = 0;

	virtual std::vector<Point> GetPointDistributionInArea(Number count) const;

	virtual bool Contains(Point point) const = 0;

	// maybe these should all return a single struct since sometimes
	// they're best computed together? see Perimeter
	virtual Point GetCenter() const = 0;

	virtual double GetArea() const = 0;

	// this currently returns a diameter, but maybe it should be a radius
	virtual double GetDistanceToFarthestPoint(Point from) const = 0;
};

struct Box : Area_Interface
{
	Point topLeft;
	Point bottomRight;

	// this is intended to only be used by value_ptr internals
	virtual Area_Interface * clone() const override
	{
		return new Box(*this);
	}

	bool Contains(Point point) const override;

	Point GetCenter() const override;

	double GetDistanceToFarthestPoint(Point from) const override;

	double GetArea() const override;
};

struct Circle : Area_Interface
{
	Point center;
	Number radius;

	// this is intended to only be used by value_ptr internals
	virtual Area_Interface * clone() const override
	{
		return new Circle(*this);
	}

	bool Contains(Point point) const override;

	Point GetCenter() const override;

	double GetDistanceToFarthestPoint(Point from) const override;

	double GetArea() const override;
};

struct Perimeter : Area_Interface
{
	Line perimeter;

	// this is intended to only be used by value_ptr internals
	virtual Area_Interface * clone() const override
	{
		return new Perimeter(*this);
	}

	bool Contains(Point point) const override;

	Point GetCenter() const override;

	double GetDistanceToFarthestPoint(Point from) const override;

	double GetArea() const override;
};

// @Incomplete perhaps Union and Intersection should be done by
// constructing a new perimeter approximation rather than
// by trying to compute
// for now, we're going to assume that Area_Union is disjoint
// and that if it were intersecting, we would just have just created
// a new perimeter approximation
// which means that Area_Intersection isn't necessary
struct Area_Union : Area_Interface
{
	// can these be disjoint?
	std::vector<value_ptr<Area_Interface>> areas;

	// this is intended to only be used by value_ptr internals
	virtual Area_Union * clone() const override
	{
		return new Area_Union(*this);
	}

	bool Contains(Point point) const override;

	Point GetCenter() const override;

	double GetDistanceToFarthestPoint(Point from) const override;

	double GetArea() const override;
};

/*
struct Area_Intersection : Area_Interface
{
	// what happens if these are disjoint?
	std::vector<std::unique_ptr<Area_Interface>> areas;

	bool Contains(Point point) const;

	Point GetCenter() const;

	double GetDistanceToFarthestPoint(Point from) const;

	double GetArea() const;
};
*/

// should this be a union type?
struct Area
{
	std::variant<Box, Circle, Perimeter, Area_Union> implementation;

	Area_Interface * interface;

	// could also consider just value_ptr<Area_Interface> implementation;

	Area(std::variant<Box, Circle, Perimeter, Area_Union> implementation)
		: implementation(implementation)
		, interface(std::visit([](auto& arg){
			return static_cast<Area_Interface*>(&arg);
		}, implementation))
	{ }

	std::vector<Point> GetPointDistributionInArea(Number count)
	{
		return interface->GetPointDistributionInArea(count);
	}

	bool Contains(Point point)
	{
		return interface->Contains(point);
	}
};

using Location = std::variant<Point, Line, Direction, Area>;

using Value = std::variant<
	Success,
	UnitGroup,
	Filter,
	GroupSize,
	Superlative,
	Number,
	Digit,
	Unit_Type,
	Ability_Type,
	Attribute_Type,
	Location,
	Point,
	Line,
	Direction,
	Area>;

template<typename TVal>
ElementType::Enum GetElementType()
{
	if constexpr(std::is_same_v<TVal, Number>)
	{
		return ElementType::Number;
	}
	else if constexpr(std::is_same_v<TVal, Digit>)
	{
		return ElementType::Digit;
	}
	else if constexpr(std::is_same_v<TVal, Unit_Type>)
	{
		return ElementType::Unit_Type;
	}
	else if constexpr(std::is_same_v<TVal, Ability_Type>)
	{
		return ElementType::Ability_Type;
	}
	else if constexpr(std::is_same_v<TVal, Attribute_Type>)
	{
		return ElementType::Attribute_Type;
	}
	else if constexpr(std::is_same_v<TVal, Point>)
	{
		return ElementType::Point;
	}
	else if constexpr(std::is_same_v<TVal, Line>)
	{
		return ElementType::Line;
	}
	else if constexpr(std::is_same_v<TVal, Direction>)
	{
		return ElementType::Direction;
	}
	else if constexpr(std::is_same_v<TVal, Area>)
	{
		return ElementType::Area;
	}
}

// relying on the declaration parameter type checking
// to verify that there is at least one for repeatable
// but not optional parameters
/*
template<typename T>
using Repeatable = std::vector<T>;

template<typename T>
using OptionalRepeatable = std::vector<T>;

// this optional should only be used for things that have
// no default value, since if there is a default
// we just pass that to the function instead
template<typename T>
using Optional = std::optional<T>;

template<typename... Ts>
using One_Of = std::variant<Ts...>;



template<typename TValue>
struct Underlying
{
	using Type = TValue;
};

template<typename TValue>
struct Underlying<std::vector<TValue> >
{
	using Type = TValue;
};
*/

} // namespace Command

#endif // BRUSHLINK_COMMAND_VALUE_TYPES_H

