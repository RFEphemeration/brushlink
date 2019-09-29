#ifndef BRUSHLINK_COMMAND_TYPES_HPP
#define BRUSHLINK_COMMAND_TYPES_HPP

using namespace Farb;

namespace Command
{

// todo: interactive visualizations


enum class ElementType
{
	Command =              1 << 0

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
	Parameter_Reference    1 << 21,

	// this is probably not an appropriate type because of the way fields are compared
	// but really I need to think about how to compare fields more thoroughly
	// which probably means not using a field, and using sets instead
	// User_Defined =         1 << 22
}

struct ElementNameTag
{
	static HString GetName() { return "Command::ElementName"; }
}
using ElementName = NamedType<HString, ElementNameTag>;

struct ElementToken
{
	// this is a bit field of flags, aka a set of types
	// but I think it should actually only be a single one
	ElementType type;
	HString name;

	bool IsType(ElementType other)
	{
		return other & type;
	}
}


struct ElementParameter
{
	// todo: does this cover one_of variants? unit or units, area or point
	ElementType types;

	Bool optional;
	// Bool permutable; // implement this later
	// Bool repeatable; // implement this later

	// you can be optional without a default_value
	value_ptr<ElementToken> default_value;

}

// the index of a paramter in an element
// -1 is left parameter, 0 is first right parameter
struct ParameterIndexTag
{
	static HString GetName() { return "Command::ParameterIndex"; }
}
using ParameterIndex = NamedType<int, ParameterIndexTag>;

const ParameterIndex kLeftParameterIndex{-1};

const ParameterIndex kNullParameterIndex{-2};

// the index of an element in a fragment or command
struct ElementIndexTag
{
	static HString GetName() { return "Command::ElementIndex"; }
}
using ElementIndex = NamedType<int, ElementIndexTag>;

const ElementIndex kNullElementIndex{-1};

struct ElementDeclaration
{
	ElementType types = 0;
	ElementName name;

	value_ptr<ElementParameter> left_parameter; // optional
	std::vector<ElementParameter> right_parameters; // could be length 0

	bool HasLeftParamterMatching(ElementType type) const;

	ErrorOr<ElementParameter> GetParameter(ParameterIndex index) const;

	ParameterIndex GetMaxParameterIndex() const;

	ParameterIndex GetMinParameterIndex() const;

protected:
	// rmf todo: change this to a multimap
	ErrorOr<Success> FillDefaultArguments(
		std::map<ParameterIndex, ElementToken>& arguments) const;
}

struct ImpliedNodeOptions
{
	static const ElementToken selectorToken { ElementType::Selector, "Selector" };
	static const ElementToken locationToken { ElementType::Location, "Location" };

	// accepted arg type -> token to use for node
	static const std::unordered_map<ElementType, ElementToken> acceptedArgTypes
	{
		{ ElementType::Selector_Base, selectorToken },
		{ ElementType::Selector_Group_Size, selectorToken },
		{ ElementType::Selector_Generic, selectorToken },
		{ ElementType::Selector_Superlative, selectorToken },

		{ ElementType::Point, locationToken },
		{ ElementType::Line, locationToken },
		{ ElementType::Area, locationToken }
	};

	// parameter type -> arg types
	static const std::unordered_map<ElementType, ElementType> potentialParamTypes
	{
		{
			ElementType::Selector,

			ElementType::Selector_Base
			| ElementType::Selector_Group_Size
			| ElementType::Selector_Generic
			| ElementType::Selector_Superlative
		},
		{
			ElementType::Location,

			ElementType::Point
			| ElementType::Line
			| ElementType::Area
		}
	}
}

// For an AST, ElementIndexes are in relationship to parent
struct ElementNode
{
	// kNullElementIndex (-1) means you are not in the stream, i.e. implied
	ElementIndex streamIndex;
	ElementToken token;

	// in order of appending, aka streamIndex
	std::list<ElementNode> children;

	// ElementIndex here refers to in the list of children
	std::multimap<ParameterIndex, ElementIndex> childArgumentMapping;

	ElementNode* parent;

	ElementNode(ElementToken token, ElementIndex streamIndex)
		: token(token)
		, streamIndex(streamIndex)
		, parent(nullptr)
	{ }

	ErrorOr<ElementNode &> Add(ElementNode child, ParameterIndex argIndex = kNullParameterIndex);

	ParameterIndex RemoveLastChild();

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
	}

	ArgAndParamWalkResult WalkArgsAndParams(ElementType nextTokenType = ElementType { 0 }) const;
}

struct Parser
{
	std::vector<ElementToken> stream;
	ElementNode root;

	ElementNode * GetRightmostElement() const;

	ErrorOr<Sucess> Append(ElementToken nextToken);


	struct ASTWalkResult
	{
		bool foundValidLocation;

		// if we found a valid location, this might be incomplete
		struct
		{
			ElementType validNextArgs;
			ElementType rightSideTypesForLeftParameter;
		} potential;

		struct
		{
			// this pointer will only be valid until we update the tree
			// after which it will be meaningless/dangling
			ElementNode * e;
			bool isLeftParam { false };

			// nothing past here is used for left params
			ParameterIndex argIndex { kNullParameterIndex };
			bool argRequiresImpliedNode { false };
		} tokenLocation;

		void AddNodeWalkResult(
			ElementNode * node,
			ElementNode::ArgAndParamWalkResult nodeWalkResult);

		void Parser::ASTWalkResult::AddTypesForPotentialLeftParams(
			ElementNode * node,
			ElementDeclaration * declaration);
	}

private:

	ASTWalkResult WalkAST(ElementToken * nextToken) const;

	ElementNode * GetValidParent(ElementToken newToken) const;

	ElementType GetRightSideTypesForLeftParameter() const;

	// remember it's also valid to append anything that has a left parameter
	// that is on the right
	ElementType GetValidNextArguments() const;
}

struct FramentMapping
{
	std::vector<ElementMapping> elementMapping;
	std::vector<ElementIndex> evaluationOrder;

	FragmentMapping(const std::vector<CRef<Element> > & elements);

	void Append(const Element & e, bool evaluateOrder = true);

	// this is recursive descent
	// requires precondition that everything in evaluationOrder so far
	// isn't dependent on the element at the provided index
	void EvaluateOrderFrom(ElementIndex index);

	ErrorOr<Success> EvaluateOrder();

	std::set<ValueType> ComputeValidAppendTypes() const;

	ElementIndex FindAppropriateLeftArgument(const Element & e) const;

	std::Pair<ElementIndex, ParameterIndex> FindAppropriateParentForNew(const Element & e) const;

	static ErrorOr<FragmentMapping> CreateMapping(const std::vector<CRef<Element> > &elements)
	{
		return FragmentMapping{elements};
	}
}

} // namespace Command

#endif // BRUSHLINK_COMMAND_TYPES_HPP
