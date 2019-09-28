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

	User_Defined =         1 << 22
}

struct ElementNameTag
{
	static HString GetName() { return "Command::ElementName"; }
}
using ElementName = NamedType<HString, ElementNameTag>;

struct ElementToken
{
	// this is a bit field of flags, aka a set of types
	ElementType types;
	HString name;

	bool IsType(ElementType other)
	{
		return other & types;
	}

	bool IsAllTypes(ElementType other)
	{
		 return (other & types) >= other;
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

// the index of an element in a fragment or command
struct ElementIndexTag
{
	static HString GetName() { return "Command::ElementIndex"; }
}
using ElementIndex = NamedType<int, ElementIndexTag>;

const ElementIndex kNullElementIndex{-1};

struct ElementDeclaration
{
	ElementType types;
	ElementName name;

	value_ptr<ElementParameter> left_parameter; // optional
	std::vector<ElementParameter> right_parameters; // could be length 0

protected:
	ErrorOr<Success> FillDefaultArguments(
		std::map<ParameterIndex, ElementToken>& arguments) const;
}


// For an AST, ElementIndexes are in relationship to parent
struct ElementNode
{
	// kNullElementIndex (-1) means you are not in the stream, i.e. implied
	ElementIndex streamIndex;
	ElementToken token;

	std::list<ElementNode> children;

	// ElementIndex here refers to in the list of children
	std::multimap<ParameterIndex, ElementIndex> childArgumentMapping;

	ElementNode* parent;

	ElementNode(ElementToken token, ElementIndex streamIndex, ElementNode* parent = nullptr)
		: token(token)
		, streamIndex(streamIndex)
		, parent(parent)
	{ }

	ErrorOr<Success> Add(ElementNode child)
	{
		auto argIndex = CHECK_RETURN(GetArgIndexForNextToken(child.token));
		auto childIndex = ElementIndex{children.size()};
		child.parent = this;
		children.push_back(child);
		childArgumentMapping.insert( { argIndex, childIndex } );
	}

	ErrorOr<ParameterIndex> GetArgIndexForNextToken(ElementToken token) const;

	// this is not recursive, doesn't guarantee arguments have ParametersMet
	bool ParametersMet() const;

	ElementType GetValidNextArguments() const;
}

struct Parser
{
	std::vector<ElementToken> stream;
	ElementNode root;
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

// made up from Atoms, Literals, and other Words
struct ElementWord : Element
{
	// TypeInfoAs std::vector<Element&>
	FramentMapping implementation;

	// we can't do recursive descent here because ElementReferences
	// must be resolved in this scope, where they can access this Word's arguments
	// even if they are arguments to other Elements in the implementation
	virtual ErrorOr<Value> Evaluate(
		CommandContext context,
		std::map<ParameterIndex, Value> arguments) const override;

private:
	ErrorOr<Success> PostLoad();
}

// for operator overloading based on parameters
struct ElementOneOf : Element
{
	
}

} // namespace Command

#endif // BRUSHLINK_COMMAND_TYPES_HPP
