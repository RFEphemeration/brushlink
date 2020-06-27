#pragma once
#ifndef BRUSHLINK_VARIANT_H
#define BRUSHLINK_VARIANT_H

#include "Action.h"
#include "Basic_Types.h"
#include "Game_Basic_Types.h"
#include "Game_Time.h"
#include "Location.h"
#include "Resources.h"

namespace Command
{

using namespace Farb;
using namespace Brushlink;

// todo: custom struct/record, sum, and tuple types
// should vector and optional be included in variant?

using Variant = std::variant<
	Success,
	Number,
	Digit,
	ValueName,
	Letter,
	Seconds,
	Action_Type,
	Unit_Type,
	Unit_Attribute,	
	UnitID,
	Unit_Group,
	Energy,
	Point,
	Direction,
	Line,
	Area>;

enum class Variant_Type
{
	Success,
	Number,
	Digit,
	ValueName,
	Letter,
	Seconds,
	Action_Type,
	Action_Step,
	Unit_Type,
	Unit_Attribute,
	UnitID,
	Unit_Group,
	Energy,
	Point,
	Direction,
	Line,
	Area
};

template<typename TVal>
Variant_Type GetVariantType()
{
	if constexpr(std::is_same_v<TVal, Success>)
		return Variant_Type::Success;
	else if constexpr(std::is_same_v<TVal, Number>)
		return Variant_Type::Number;
	else if constexpr(std::is_same_v<TVal, Digit>)
		return Variant_Type::Digit;
	else if constexpr(std::is_same_v<TVal, ValueName>)
		return Variant_Type::ValueName;
	else if constexpr(std::is_same_v<TVal, Letter>)
		return Variant_Type::Letter;
	else if constexpr(std::is_same_v<TVal, Seconds>)
		return Variant_Type::Seconds;
	else if constexpr(std::is_same_v<TVal, Action_Type>)
		return Variant_Type::Action_Type;
	else if constexpr(std::is_same_v<TVal, Action_Step>)
		return Variant_Type::Action_Step;
	else if constexpr(std::is_same_v<TVal, Unit_Type>)
		return Variant_Type::Unit_Type;
	else if constexpr(std::is_same_v<TVal, Unit_Attribute>)
		return Variant_Type::Unit_Attribute;
	else if constexpr(std::is_same_v<TVal, UnitID>)
		return Variant_Type::UnitID;
	else if constexpr(std::is_same_v<TVal, Unit_Group>)
		return Variant_Type::Unit_Group;
	else if constexpr(std::is_same_v<TVal, Energy>)
		return Variant_Type::Energy;
	else if constexpr(std::is_same_v<TVal, Point>)
		return Variant_Type::Point;
	else if constexpr(std::is_same_v<TVal, Direction>)
		return Variant_Type::Direction;
	else if constexpr(std::is_same_v<TVal, Line>)
		return Variant_Type::Line;
	else if constexpr(std::is_same_v<TVal, Area>)
		return Variant_Type::Area;
}


inline Variant_Type GetVariantType(Variant v)
{
	if std::holds_alternative<Success>(v)
		return Variant_Type::Success
	else if std::holds_alternative<Number>(v)
		return Variant_Type::Number
	else if std::holds_alternative<Digit>(v)
		return Variant_Type::Digit
	else if std::holds_alternative<ValueName>(v)
		return Variant_Type::ValueName
	else if std::holds_alternative<Letter>(v)
		return Variant_Type::Letter
	else if std::holds_alternative<Seconds>(v)
		return Variant_Type::Seconds
	else if std::holds_alternative<Action_Type>(v)
		return Variant_Type::Action_Type
	else if std::holds_alternative<Action_Step>(v)
		return Variant_Type::Action_Step
	else if std::holds_alternative<Unit_Type>(v)
		return Variant_Type::Unit_Type
	else if std::holds_alternative<Unit_Attribute>(v)
		return Variant_Type::Unit_Attribute
	else if std::holds_alternative<UnitID>(v)
		return Variant_Type::UnitID
	else if std::holds_alternative<Unit_Group>(v)
		return Variant_Type::Unit_Group
	else if std::holds_alternative<Energy>(v)
		return Variant_Type::Energy
	else if std::holds_alternative<Point>(v)
		return Variant_Type::Point
	else if std::holds_alternative<Direction>(v)
		return Variant_Type::Direction
	else if std::holds_alternative<Line>(v)
		return Variant_Type::Line
	else if std::holds_alternative<Area>(v)
		return Variant_Type::Area
}

} // namespace Brushlink

#endif // BRUSHLINK_VARIANT_H


