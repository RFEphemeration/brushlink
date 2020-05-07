#ifndef BRUSHLINK_COMMAND_TYPES_HPP
#define BRUSHLINK_COMMAND_TYPES_HPP

#include <vector>
#include <unordered_map>
#include <map>
#include <list>

#include "BuiltinTypedefs.h"
#include "NamedType.hpp"
#include "ErrorOr.hpp"

using namespace Farb;

namespace Command
{

// todo: interactive visualizations

namespace ElementType
{
enum Enum
{
	Command =              1 << 0,

	Condition =            1 << 1,
	Action =               1 << 2,

	Selector =             1 << 3,
	Set =                  1 << 4, // base, group, crew, set?
	Filter =               1 << 5, // predicate?
	Group_Size =           1 << 6,
	Superlative =          1 << 7,

	Location =             1 << 8,
	Point =                1 << 9,
	Line =                 1 << 10,
	Area =                 1 << 11,
	Direction =            1 << 12,

	// Direction?

	Unit_Type =            1 << 13,
	Attribute_Type =       1 << 14, // drop _Type?
	Ability_Type =         1 << 15, // skill?
	Resource_Type =        1 << 16,

	Number =               1 << 17,

	// @Cleanup maybe combine into single type Instruction? Punctuation?
	// would require handling Allowed slightly differently
	Skip =                 1 << 18,
	Termination =          1 << 19,
	Cancel =               1 << 20,
	Undo =                 1 << 21,
	Redo =                 1 << 22,
	// RepeatLastCommand
	// Begin_Word? End_Word?

	Mouse_Input =          1 << 23,

	Parameter_Reference =  1 << 24

	// these are probably not appropriate types because of the way fields are compared
	// but really I need to think about how to compare fields more thoroughly
	// which probably means not using a field, and using sets instead
	// or these become flags

	//Default_Value =        1 << 18,
	//Implied =              1 << 19,

	// has no parameters and is not context dependent
	//Literal =              1 << 20,

	// User_Defined =         1 << 22
};

} // namespace ElementType

const ElementType::Enum kNullElementType = static_cast<ElementType::Enum>(0);

inline ElementType::Enum operator|(ElementType::Enum a, ElementType::Enum b)
{
	return static_cast<ElementType::Enum>(static_cast<int>(a) | static_cast<int>(b));
}

inline ElementType::Enum operator&(ElementType::Enum a, ElementType::Enum b)
{
	return static_cast<ElementType::Enum>(static_cast<int>(a) & static_cast<int>(b));
}

inline std::string ToString(ElementType::Enum a)
{
	switch(a)
	{
	case ElementType::Command:
		return "Command";
	case ElementType::Condition:
		return "Condition";
	case ElementType::Action:
		return "Action";
	case ElementType::Selector:
		return "Selector";
	case ElementType::Set:
		return "Set";
	case ElementType::Filter:
		return "Filter";
	case ElementType::Group_Size:
		return "Group_Size";
	case ElementType::Superlative:
		return "Superlative";
	case ElementType::Location:
		return "Location";
	case ElementType::Point:
		return "Point";
	case ElementType::Line:
		return "Line";
	case ElementType::Area:
		return "Area";
	case ElementType::Direction:
		return "Direction";
	case ElementType::Unit_Type:
		return "Unit_Type";
	case ElementType::Attribute_Type:
		return "Attribute_Type";
	case ElementType::Ability_Type:
		return "Ability_Type";
	case ElementType::Resource_Type:
		return "Resource_Type";
	case ElementType::Number:
		return "Number";
	case ElementType::Skip:
		return "Skip";
	case ElementType::Undo:
		return "Undo";
	case ElementType::Redo:
		return "Redo";
	case ElementType::Termination:
		return "Termination";
	case ElementType::Cancel:
		return "Cancel";
	case ElementType::Mouse_Input:
		return "Mouse_Input";
	case ElementType::Parameter_Reference:
		return "Parameter_Reference";
	default:
		return "";
	}
}

// rmf todo: split out ElementFlags from ElementType


struct ElementNameTag
{
	static HString GetName() { return "Command::ElementName"; }
};
using ElementName = NamedType<HString, ElementNameTag>;

struct ElementToken
{
	// this is a bit field of flags, aka a set of types
	// but I think it should actually only be a single one
	ElementType::Enum type;
	ElementName name;
	bool has_left_param;
	// could also consider bool is_literal for combining literals

	ElementToken(ElementType::Enum type, ElementName name, bool has_left_param = false)
		: name(name)
		, type(type)
		, has_left_param(has_left_param)
	{ }

	bool IsType(ElementType::Enum other)
	{
		return static_cast<uint>(other) & static_cast<uint>(type);
	}
};


namespace OccurrenceFlags
{
	enum Enum
	{
		Optional =   1 << 0,
		Permutable = 1 << 1, // unimplemented
		Repeatable = 1 << 2,
		Implied =    1 << 3, // used to automatically instantiate default_value
	};
}

inline OccurrenceFlags::Enum operator|(OccurrenceFlags::Enum a, OccurrenceFlags::Enum b)
{
	return static_cast<OccurrenceFlags::Enum>(static_cast<int>(a) | static_cast<int>(b));
}

} // namespace Command

#endif // BRUSHLINK_COMMAND_TYPES_HPP
