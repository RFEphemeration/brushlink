#pragma once
#ifndef BRUSHLINK_VARIANT_H
#define BRUSHLINK_VARIANT_H

#include "BuiltinTypedefs.h"

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
// should vector and/or optional be included in variant?
// what about Element and Parameter?

using Variant = std::variant<
	Success,
	Bool,
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
	Area>;

enum class Variant_Type
{
	Success,
	Bool,
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
	Area,
	Any,
};

const Set<Variant_Type> all_variant_types {
	Variant_Type::Success,
	Variant_Type::Bool,
	Variant_Type::Number,
	Variant_Type::Digit,
	Variant_Type::ValueName,
	Variant_Type::Letter,
	Variant_Type::Seconds,
	Variant_Type::Action_Type,
	Variant_Type::Action_Step,
	Variant_Type::Unit_Type,
	Variant_Type::Unit_Attribute,
	Variant_Type::UnitID,
	Variant_Type::Unit_Group,
	Variant_Type::Energy,
	Variant_Type::Point,
	Variant_Type::Direction,
	Variant_Type::Line,
	Variant_Type::Area,
	Variant_Type::Any,
};

template<typename TVal>
Variant_Type GetVariantType()
{
	if constexpr(std::is_same_v<TVal, Success>)
		return Variant_Type::Success;
	else if constexpr(std::is_same_v<TVal, Bool>)
		return Variant_Type::Bool;
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
	return Variant_Type::Any;
}


inline Variant_Type GetVariantType(Variant v)
{
	if (std::holds_alternative<Success>(v))
		return Variant_Type::Success;
	else if (std::holds_alternative<Bool>(v))
		return Variant_Type::Bool;
	else if (std::holds_alternative<Number>(v))
		return Variant_Type::Number;
	else if (std::holds_alternative<Digit>(v))
		return Variant_Type::Digit;
	else if (std::holds_alternative<ValueName>(v))
		return Variant_Type::ValueName;
	else if (std::holds_alternative<Letter>(v))
		return Variant_Type::Letter;
	else if (std::holds_alternative<Seconds>(v))
		return Variant_Type::Seconds;
	else if (std::holds_alternative<Action_Type>(v))
		return Variant_Type::Action_Type;
	else if (std::holds_alternative<Action_Step>(v))
		return Variant_Type::Action_Step;
	else if (std::holds_alternative<Unit_Type>(v))
		return Variant_Type::Unit_Type;
	else if (std::holds_alternative<Unit_Attribute>(v))
		return Variant_Type::Unit_Attribute;
	else if (std::holds_alternative<UnitID>(v))
		return Variant_Type::UnitID;
	else if (std::holds_alternative<Unit_Group>(v))
		return Variant_Type::Unit_Group;
	else if (std::holds_alternative<Energy>(v))
		return Variant_Type::Energy;
	else if (std::holds_alternative<Point>(v))
		return Variant_Type::Point;
	else if (std::holds_alternative<Direction>(v))
		return Variant_Type::Direction;
	else if (std::holds_alternative<Line>(v))
		return Variant_Type::Line;
	else if (std::holds_alternative<Area>(v))
		return Variant_Type::Area;
	return Variant_Type::Any;
}

inline std::string ToString(Variant_Type v)
{
	switch(v)
	{
	case Variant_Type::Success:
		return "Success";
	case Variant_Type::Bool:
		return "Bool";
	case Variant_Type::Number:
		return "Number";
	case Variant_Type::Digit:
		return "Digit";
	case Variant_Type::ValueName:
		return "ValueName";
	case Variant_Type::Letter:
		return "Letter";
	case Variant_Type::Seconds:
		return "Seconds";
	case Variant_Type::Action_Type:
		return "Action_Type";
	case Variant_Type::Action_Step:
		return "Action_Step";
	case Variant_Type::Unit_Type:
		return "Unit_Type";
	case Variant_Type::Unit_Attribute:
		return "Unit_Attribute";
	case Variant_Type::UnitID:
		return "UnitID";
	case Variant_Type::Unit_Group:
		return "Unit_Group";
	case Variant_Type::Energy:
		return "Energy";
	case Variant_Type::Point:
		return "Point";
	case Variant_Type::Direction:
		return "Direction";
	case Variant_Type::Line:
		return "Line";
	case Variant_Type::Area:
		return "Area";
	case Variant_Type::Any:
		return "Any";
	}
	return "Any";
}

inline Variant_Type FromString(std::string s)
{
	if (s == "Success")
		return Variant_Type::Success;
	else if (s == "Bool")
		return Variant_Type::Bool;
	else if (s == "Number")
		return Variant_Type::Number;
	else if (s == "Digit")
		return Variant_Type::Digit;
	else if (s == "ValueName")
		return Variant_Type::ValueName;
	else if (s == "Letter")
		return Variant_Type::Letter;
	else if (s == "Seconds")
		return Variant_Type::Seconds;
	else if (s == "Action_Type")
		return Variant_Type::Action_Type;
	else if (s == "Action_Step")
		return Variant_Type::Action_Step;
	else if (s == "Unit_Type")
		return Variant_Type::Unit_Type;
	else if (s == "Unit_Attribute")
		return Variant_Type::Unit_Attribute;
	else if (s == "UnitID")
		return Variant_Type::UnitID;
	else if (s == "Unit_Group")
		return Variant_Type::Unit_Group;
	else if (s == "Energy")
		return Variant_Type::Energy;
	else if (s == "Point")
		return Variant_Type::Point;
	else if (s == "Direction")
		return Variant_Type::Direction;
	else if (s == "Line")
		return Variant_Type::Line;
	else if (s == "Area")
		return Variant_Type::Area;
	else if (s == "Any")
		return Variant_Type::Any;
	return Variant_Type::Any;
}


} // namespace Command

namespace std
{
template<>
class hash<Farb::Set<Command::Variant_Type>>
{
public:
	size_t operator()(const Farb::Set<Command::Variant_Type> & set) const
	{
		size_t sum = 0;
		for(auto type : set)
		{
			sum += static_cast<size_t>(1 << static_cast<int>(type));
		}
		return sum;
	}
};

} // namespace std

#endif // BRUSHLINK_VARIANT_H


