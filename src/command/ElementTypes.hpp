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


enum class ElementType
{
	Command =              1 << 0,

	Condition =            1 << 1,
	Action =               1 << 2,

	Selector =             1 << 3,
	Selector_Base =        1 << 4,
	Selector_Generic =     1 << 5,
	Selector_Group_Size =  1 << 6,
	Selector_Superlative = 1 << 7,

	Location =             1 << 8,
	Point =                1 << 9,
	Line =                 1 << 10,
	Area =                 1 << 11,

	Unit_Type =            1 << 12,
	Attribute_Type =       1 << 13,
	Ability_Type =         1 << 14,

	Number =               1 << 15,

	Skip =                 1 << 16,
	Termination =          1 << 17,

	// Begin_Word? End_Word?

	Default_Value =        1 << 18,
	Implied =              1 << 19,

	// has no parameters and is not context dependent
	Literal =              1 << 20,
	Parameter_Reference =  1 << 21

	// this is probably not an appropriate type because of the way fields are compared
	// but really I need to think about how to compare fields more thoroughly
	// which probably means not using a field, and using sets instead
	// User_Defined =         1 << 22
};

inline ElementType operator|(ElementType a, ElementType b)
{
	return static_cast<ElementType>(static_cast<int>(a) | static_cast<int>(b));
}

inline ElementType operator&(ElementType a, ElementType b)
{
	return static_cast<ElementType>(static_cast<int>(a) & static_cast<int>(b));
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
	ElementType type;
	ElementName name;

	ElementToken(ElementName name, ElementType type)
		: name(name)
		, type(type)
	{ }

	bool IsType(ElementType other)
	{
		return static_cast<uint>(other) & static_cast<uint>(type);
	}
};


struct ElementParameter
{
	// todo: does this cover one_of variants? unit or units, area or point
	ElementType types;

	bool optional = false;
	bool permutable = false; // implement this later
	bool repeatable = false; // implement this later

	// you can be optional without a default_value
	value_ptr<ElementToken> default_value { nullptr };

	ElementParameter(ElementType type, bool optional = false, bool permutable = false, bool repeatable = false)
		: types(type)
		, optional(optional)
	{ }

	ElementParameter(ElementType type, ElementToken default_value)
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

struct ElementDeclaration
{
	ElementType types { 0 };
	ElementName name;

	value_ptr<ElementParameter> left_parameter; // optional
	std::vector<ElementParameter> right_parameters; // could be length 0

	ElementDeclaration(ElementName name, ElementType type)
		: name(name)
		, types(type)
		, left_parameter(nullptr)
		, right_parameters()
	{ }

	ElementDeclaration(ElementName name, ElementType type, std::vector<ElementParameter> right_parameters)
		: name(name)
		, types(type)
		, left_parameter(nullptr)
		, right_parameters(right_parameters)
	{ }

	ElementDeclaration(ElementName name, ElementType type, ElementParameter left_parameter, std::vector<ElementParameter> right_parameters)
		: name(name)
		, types(type)
		, left_parameter(left_parameter)
		, right_parameters(right_parameters)
	{ }

	bool HasLeftParamterMatching(ElementType type) const;

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
	static const std::unordered_map<ElementType, ElementToken> acceptedArgTypes;

	// parameter type -> arg types
	static const std::unordered_map<ElementType, ElementType> potentialParamTypes;
};

// For an AST, ElementIndexes are in relationship to parent
struct ElementNode
{
	// kNullElementIndex (-1) means you are not in the stream, i.e. implied
	ElementIndex streamIndex = kNullElementIndex;
	ElementToken token;

	// in order of appending, aka streamIndex
	// maybe used owned_ptr?
	std::vector<ElementNode> children;

	// ElementIndex here refers to in the list of children
	// should probably just have the map directly go to ElementNode
	// or not becausde the last element added is important, and undos make this chain
	// all the way back
	std::multimap<ParameterIndex, ElementIndex> childArgumentMapping;

	ElementNode* parent = nullptr;

	ElementNode(ElementToken token, ElementIndex streamIndex)
		: token(token)
		, streamIndex(streamIndex)
		, parent(nullptr)
	{ }

	static bool Equal(const ElementNode & a, const ElementNode & b);

	static std::string GetPrintString(const ElementNode & e, std::string indentation = "", ParameterIndex argIndex = kNullParameterIndex);

	ErrorOr<ElementNode *> Add(ElementNode child, ParameterIndex argIndex = kNullParameterIndex);

	ParameterIndex RemoveLastChild();

	void FillDefaultArguments();

	void UpdateChildrenSetParent();

	// these are not recursive, don't guarantee arguments have ParametersMet
	// these three should only be used when you only need one of them
	// if you need two or more, you should call WalkArgsAndParams
	// since these are just wrappers around that
	bool ParametersMet() const;
	ElementType GetValidNextArgTypes() const;
	ElementType GetValidNextArgsWithImpliedNodes() const;
	ErrorOr<ParameterIndex> GetArgIndexForNextToken(ElementToken token) const;

	struct ArgAndParamWalkResult
	{
		bool allParametersMet { true };
		ElementType validNextArgs { 0 };
		ElementType validNextArgsWithImpliedNodes { 0 };
		ParameterIndex firstArgIndexForNextToken { kNullParameterIndex };
		bool firstArgRequiresImpliedNode { false };

		void AddValidNextArg(
			ElementType nextTokenType,
			ParameterIndex index,
			ElementType types);
	};

	ArgAndParamWalkResult WalkArgsAndParams(ElementType nextTokenType = ElementType { 0 }) const;
};

struct Parser
{
	struct NextTokenCriteria
	{
		ElementType validNextArgs { 0 };
		ElementType rightSideTypesForLeftParameter { 0 };
	};

	struct ASTWalkResult
	{
		ElementToken walkedWith {ElementName{""}, ElementType{0}};

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
