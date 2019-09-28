#ifndef BRUSHLINK_COMMAND_ELEMENTS_H
#define BRUSHLINK_COMMAND_ELEMENTS_H

namespace Command
{

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

	User_Defined =         1 << 21
}

struct ElementToken
{
	// this is a bit field of flags, aka a set of types
	ElementType type;
	HString name;

	bool IsType(ElementType other)
	{
		return other & type;
	}

	bool IsAllTypes(ElementType other)
	{
		 return (other & type) >= other;
	}
}

struct ParseResult
{
	ElementType validNextElementTypes;
	bool complete;

	ParseResult(ElementType next, bool complete = false)
		: validNextElementTypes(next)
		, complete(complete)
	{ }

	ParseResult(Success)
		: validNextElementTypes(0)
		, complete(true)
	{ }
}

struct ElementParameter
{
	ElementType type;
	Bool permutable;
	ElementToken default_value;
}

using ElementName = NamedType<HString, NAMED_TYPE_TAG(ElementName)>;

struct ElementDefinition
{
	ElementType type;
	ElementName name;

	std::vector<ElementParameter> parameters;
}

struct ElementNode
{
	ElementToken token;
	int level = 0;
	std::vector<ElementNode> children;

	ElementNode* parent;

	ElementNode(ElementToken token)
		: token(token)
	{ }

	ElementNode(const ElementNode & other)
		: token (other.token)
		, level (other.level)
		, children (other.children)
	{ }

	void Add(ElementToken newToken)
	{
		Add(ElementNode{ newToken });
	}

	void Add(ElementNode child)
	{
		child.level = level + 1;
		children.push_back(child);
	}
}


class Parser
{
	std::vector<ElementToken> tokens;
	int tokenIndex = 0;

	ElementNode ast = ElementNode{ElementType::Command};

	inline ElementToken & token()
	{
		return tokens[tokenIndex];
	}

	ErrorOr<ParseResult> Consume(ElementType type)
	{
		if (tokenIndex >= tokens.size())
		{
			return ParseResult { type };
		}
		if (token().IsType(type))
		{
			tokenIndex++;
			return ParseResult { Success() };
		}
		return Error("Expected type " + ToString(type) + " but got " + ToString(token()));
	}

	ErrorOr<ParseResult> ParseUsingDefinition(ErrorOr<ParseResult> (*definition)(void))
	{
		int startIndex = tokenIndex;
		auto result = definition();
		if (result.IsError())
		{
			// rewind
			tokenIndex = startIndex;
			return result.GetError();
		}
		auto value = result.GetValue();
		if (!value.complete)
		{
			// rewind
			tokenIndex = startIndex;
		}
		return result.GetValue();
	}

	ParseResult Command()
	{
		ast = ElementNode{};
	}

	ParseResult Selector()
	{

	}
}


} // namespace Command

#endif // BRUSHLINK_COMMAND_ELEMENTS_H