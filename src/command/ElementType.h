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

	Skip =                 1 << 18,
	Termination =          1 << 19,
	// Begin_Word? End_Word?

	Mouse_Input =          1 << 20,

	Parameter_Reference =  1 << 21

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

	ElementToken(ElementName name, ElementType::Enum type)
		: name(name)
		, type(type)
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
		Permutable = 1 << 1,
		Repeatable = 1 << 2
	};
}

inline OccurrenceFlags::Enum operator|(OccurrenceFlags::Enum a, OccurrenceFlags::Enum b)
{
	return static_cast<OccurrenceFlags::Enum>(static_cast<int>(a) | static_cast<int>(b));
}

} // namespace Command

#endif // BRUSHLINK_COMMAND_TYPES_HPP
