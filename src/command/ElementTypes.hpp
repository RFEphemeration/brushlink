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

	// Direction?

	Unit_Type =            1 << 12,
	Attribute_Type =       1 << 13, // drop _Type?
	Ability_Type =         1 << 14, // skill?
	Resource_Type =        1 << 15,

	Number =               1 << 16,

	Skip =                 1 << 17,
	Termination =          1 << 18,
	// Begin_Word? End_Word?

	Mouse_Input =          1 << 19,

	Parameter_Reference =  1 << 20

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

	ElementToken(ElementName name);

	ElementToken(ElementName name, ElementType::Enum type)
		: name(name)
		, type(type)
	{ }

	bool IsType(ElementType::Enum other)
	{
		return static_cast<uint>(other) & static_cast<uint>(type);
	}
};

enum OccurrenceFlags
{
	Optional =   1 << 0,
	Permutable = 1 << 1,
	Repeatable = 1 << 2
};


inline OccurrenceFlags operator|(OccurrenceFlags a, OccurrenceFlags b)
{
	return static_cast<OccurrenceFlags>(static_cast<int>(a) | static_cast<int>(b));
}


struct ElementParameter
{
	// todo: does this cover one_of variants? unit or units, area or point
	ElementType::Enum types;

	bool optional = false;
	bool permutable = false; // implement this later
	bool repeatable = false; // implement this later

	// you can be optional without a default_value
	// this is a list of elements that should be parsed into a tree
	value_ptr<ElementName> default_value;

	ElementParameter(ElementType::Enum type, OccurrenceFlags flags = {})
		: types(type)
		, optional(flags & Optional)
		, permutable(flags & Permutable)
		, repeatable(flags & Repeatable)
	{ }

	ElementParameter(ElementType::Enum type, ElementName default_value)
		: types(type)
		, optional(true)
		, default_value(default_value)
	{ }
};

// the index of a paramter in an element
// -1 is left parameter, 0 is first right parameter
struct ParameterIndexTag
{
	static HString GetName() { return "Command::ParameterIndex"; }
};
using ParameterIndex = NamedType<int, ParameterIndexTag>;

const ParameterIndex kLeftParameterIndex{-1};

const ParameterIndex kNullParameterIndex{-2};

// the index of an element in a fragment or command

struct ElementIndexTag
{
	static HString GetName() { return "Command::ElementIndex"; }
};

using ElementIndex = NamedType<int, ElementIndexTag>;

const ElementIndex kNullElementIndex{-1};

struct LeftParameterTag
{
	static HString GetName() { return "Command::LeftParameter"; }
};

using LeftParameter = NamedType<ElementParameter, LeftParameterTag>;

struct ElementDeclaration
{
	ElementType::Enum types = kNullElementType;
	ElementName name;

	value_ptr<ElementParameter> left_parameter = nullptr; // optional
	std::vector<ElementParameter> right_parameters; // could be length 0

	OwnedPtr<ElementDefinition> definition;

	ElementDeclaration(ElementName name, ElementType::Enum type, OwnedPtr<ElementDefinition>&& definition)
		: name(name)
		, types(type)
		, definition(definition)
	{ }

	ElementDeclaration(
		ElementName name,
		ElementType::Enum type,
		OwnedPtr<ElementDefinition>&& definition,
		std::vector<ElementParameter> right_parameters)
		: name(name)
		, types(type)
		, right_parameters(right_parameters)
		, definition(definition)
	{ }

	ElementDeclaration(ElementName name, ElementType::Enum type, OwnedPtr<ElementDefinition>&& definition, LeftParameter left_parameter)
		: name(name)
		, types(type)
		, left_parameter(left_parameter.value)
		, definition(definition)
	{ }

	ElementDeclaration(ElementName name, ElementType::Enum type, OwnedPtr<ElementDefinition>&& definition, LeftParameter left_parameter, std::vector<ElementParameter> right_parameters)
		: name(name)
		, types(type)
		, left_parameter(left_parameter.value)
		, right_parameters(right_parameters)
		, definition(definition)
	{ }

	operator std::pair<ElementName, ElementDeclaration>()
	{
		return {name, *this};
	}

	bool HasLeftParamterMatching(ElementType::Enum type) const;

	ErrorOr<ElementParameter> GetParameter(ParameterIndex index) const;

	ParameterIndex GetMaxParameterIndex() const;

	ParameterIndex GetMinParameterIndex() const;

protected:
	// rmf todo: change this to a multimap
	ErrorOr<Success> FillDefaultArguments(
		std::map<ParameterIndex, ElementToken>& arguments) const;
};

} // namespace Command

#endif // BRUSHLINK_COMMAND_TYPES_HPP
