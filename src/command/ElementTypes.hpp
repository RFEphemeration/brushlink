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
	Set =                 1 << 4, // group, crew, set?
	Filter =               1 << 5, // predicate?
	Group_Size =           1 << 6,
	Superlative =          1 << 7,

	Location =             1 << 8,
	Point =                1 << 9,
	Line =                 1 << 10,
	Area =                 1 << 11,

	Unit_Type =            1 << 12,
	Attribute_Type =       1 << 13, // drop _Type?
	Ability_Type =         1 << 14, // skill?

	Number =               1 << 15,

	Skip =                 1 << 16,
	Termination =          1 << 17,
	// Begin_Word? End_Word?

	Parameter_Reference =  1 << 18

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


struct ElementParameter
{
	// todo: does this cover one_of variants? unit or units, area or point
	ElementType::Enum types;

	bool optional = false;
	bool permutable = false; // implement this later
	bool repeatable = false; // implement this later

	// you can be optional without a default_value
	value_ptr<ElementToken> default_value { nullptr };

	ElementParameter(ElementType::Enum type, bool optional = false, bool permutable = false, bool repeatable = false)
		: types(type)
		, optional(optional)
	{ }

	ElementParameter(ElementType::Enum type, ElementToken default_value)
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

	value_ptr<ElementParameter> left_parameter; // optional
	std::vector<ElementParameter> right_parameters; // could be length 0

	ElementDeclaration(ElementName name, ElementType::Enum type)
		: name(name)
		, types(type)
		, left_parameter(nullptr)
		, right_parameters()
	{ }

	ElementDeclaration(ElementName name, ElementType::Enum type, std::vector<ElementParameter> right_parameters)
		: name(name)
		, types(type)
		, left_parameter(nullptr)
		, right_parameters(right_parameters)
	{ }

	ElementDeclaration(ElementName name, ElementType::Enum type, LeftParameter left_parameter, std::vector<ElementParameter> right_parameters)
		: name(name)
		, types(type)
		, left_parameter(left_parameter.value)
		, right_parameters(right_parameters)
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


// could implied nodes also be used for parameter_references?
// since their return type is context dependent?
// or should there be different types of parameter_reference nodes?
struct ImpliedNodeOptions
{
	static const ElementToken selectorToken;
	static const ElementToken locationToken;

	// accepted arg type -> token to use for node
	static const std::unordered_map<ElementType::Enum, ElementToken> acceptedArgTypes;

	// parameter type -> arg types
	static const std::unordered_map<ElementType::Enum, ElementType::Enum> potentialParamTypes;
};

// For an AST, ElementIndexes are in relationship to parent
struct ElementNode
{
	// kNullElementIndex (-1) means you are not in the stream, i.e. implied
	ElementIndex streamIndex = kNullElementIndex;
	ElementToken token;

	// in order of appending, aka streamIndex
	// maybe used owned_ptr?
	std::vector<std::pair<ParameterIndex, ElementNode> > children;

	ElementNode* parent = nullptr;

	ElementNode(ElementToken token, ElementIndex streamIndex)
		: token(token)
		, streamIndex(streamIndex)
		, parent(nullptr)
	{ }

	static bool Equal(const ElementNode & a, const ElementNode & b);

	static std::string GetPrintString(const ElementNode & e, std::string indentation = "", ParameterIndex argIndex = kNullParameterIndex);

	int GetArgCountForParam(ParameterIndex index) const;

	ErrorOr<ElementNode *> Add(ElementNode child, ParameterIndex argIndex = kNullParameterIndex);

	ParameterIndex RemoveLastChild();

	void FillDefaultArguments();

	void UpdateChildrenSetParent();

	// these are not recursive, don't guarantee arguments have ParametersMet
	// these three should only be used when you only need one of them
	// if you need two or more, you should call WalkArgsAndParams
	// since these are just wrappers around that
	bool ParametersMet() const;
	ElementType::Enum GetValidNextArgTypes() const;
	ElementType::Enum GetValidNextArgsWithImpliedNodes() const;
	ErrorOr<ParameterIndex> GetArgIndexForNextToken(ElementToken token) const;

	struct ArgAndParamWalkResult
	{
		bool allParametersMet { true };
		ElementType::Enum validNextArgs = kNullElementType;
		ElementType::Enum validNextArgsWithImpliedNodes = kNullElementType;
		ParameterIndex firstArgIndexForNextToken { kNullParameterIndex };
		bool firstArgRequiresImpliedNode { false };

		void AddValidNextArg(
			ElementType::Enum nextTokenType,
			ParameterIndex index,
			ElementType::Enum types);
	};

	ArgAndParamWalkResult WalkArgsAndParams(ElementType::Enum nextTokenType = kNullElementType) const;
};

struct Parser
{
	struct NextTokenCriteria
	{
		ElementType::Enum validNextArgs = kNullElementType;
		ElementType::Enum rightSideTypesForLeftParameter = kNullElementType;
	};

	struct ASTWalkResult
	{
		ElementToken walkedWith {ElementName{""}, kNullElementType};

		bool completeStatement { false };
		bool foundValidLocation { false };

		// if we found a valid location, this might be incomplete
		NextTokenCriteria potential;

		struct
		{
			// this pointer will only be valid until we update the tree
			// after which it will be meaningless/dangling
			ElementNode * e { nullptr };
			bool isLeftParam { false };

			// nothing past here is used for left params
			ParameterIndex argIndex { kNullParameterIndex };
			bool argRequiresImpliedNode { false };
		} tokenLocation;

		void AddNodeWalkResult(
			ElementNode * node,
			ElementNode::ArgAndParamWalkResult nodeWalkResult);

		void AddTypesForPotentialLeftParams(
			ElementNode * node,
			const ElementDeclaration * declaration);
	};

	std::vector<ElementToken> stream;
	value_ptr<ElementNode> root;

	value_ptr<ASTWalkResult> mostRecentWalkResult;


	ErrorOr<Success> Append(ElementToken nextToken);

	NextTokenCriteria GetNextTokenCriteria();

private:

	ElementNode * GetRightmostElement();

	ASTWalkResult WalkAST(ElementToken * nextToken = nullptr,
		bool breakOnFoundLocation = false);
};

class ElementDictionary
{
	static const std::map<ElementName, ElementDeclaration> declarations;

public:
	static const ElementDeclaration * GetDeclaration(ElementName name)
	{
		return &declarations.at(name);
	}
};

} // namespace Command

#endif // BRUSHLINK_COMMAND_TYPES_HPP
