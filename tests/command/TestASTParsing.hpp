#ifndef TEST_AST_PARSING_HPP
#define TEST_AST_PARSING_HPP

#include <assert.h> 

#include "../../../farb/tests/RegisterTest.hpp"
#include "../../src/command/ElementTypes.cpp"


using namespace Farb;

using namespace Farb::Tests;

using namespace Command;

struct ElementNodeFlags
{
	bool implied = false;

	ElementNodeFlags(bool implied = false)
		:implied(implied)
	{ }
};

struct LinearElementNode
{
	ElementName name;
	ParameterIndex argIndex = kNullParameterIndex;
	int nestingLevel = 0;
	ElementNodeFlags flags;

	LinearElementNode(ElementName name, ParameterIndex argIndex, int nestingLevel, ElementNodeFlags flags = {})
		: name(name)
		, argIndex(argIndex)
		, nestingLevel(nestingLevel)
		, flags(flags)
	{ }

	bool operator==(const LinearElementNode & other) const
	{
		return name == other.name
			&& argIndex == other.argIndex
			&& nestingLevel == other.nestingLevel
			&& flags.implied == other.flags.implied;
	}
};

using LEN = LinearElementNode;

void ConvertToLEN(ElementNode node, ParameterIndex argIndex, int nestingLevel, std::vector<LEN>& linear)
{
	linear.push_back(LinearElementNode{
		node.token.name,
		argIndex,
		nestingLevel,
		{node.streamIndex == kNullElementIndex}
	});

	nestingLevel++;
	for (auto pair : node.childArgumentMapping)
	{
		ConvertToLEN(node.children[pair.second.value], pair.first, nestingLevel, linear);
	}
}

void TestASTFromStream(
	std::vector<ElementToken> & tokens,
	const std::vector<LEN> & expected_linear,
	bool complete = true)
{
	Parser parser;
	std::string stream = "";

	for (auto token : tokens)
	{
		auto result = parser.Append(token);
		if (result.IsError())
		{
			farb_print(false, "TestAST appending token " + token.name.value + " failed");
			assert(false);
		};
		stream += token.name.value + ", ";
	}
	std::vector<LEN> linear;
	ConvertToLEN(*parser.root, kNullParameterIndex, 0, linear);
	bool success = linear == expected_linear;
	//bool success = ElementNode::Equal(expected_result, *parser.root);

	std::string printString = ElementNode::GetPrintString(*parser.root, "            ");
	
	farb_print(success,
		"parsed ast from stream: [ " + stream + "]\n" + printString);
	
	assert(success);
}

using Implied = ImpliedNodeOptions;

class TestASTParsing : public ITest
{
public:
	virtual bool RunTests() const override
	{
		std::cout << "AST Parsing" << std::endl;

		std::vector<ElementToken> stream = {
			{ ElementName{"Attack"}, ElementType::Action },
			{ ElementName{"Enemies"}, ElementType::Selector_Base }
		};

		std::vector<LEN> expected_linear{
			{stream[0].name, kNullParameterIndex, 0},
			{Implied::selectorToken.name, 0, 1, {true}},
			{stream[1].name, 0, 2}
		};

		TestASTFromStream(stream, expected_linear);

		/*
		stream = {
			{ ElementName{"Attack"}, ElementType::Action },
			{ ElementName{"Enemies"}, ElementType::Selector_Base },
			{ ElementName{"Within_Range"}, ElementType::Selector_Generic }
		};

		TestASTFromStream(stream);

		stream = {
			{ ElementName{"Attack"}, ElementType::Action },
			{ ElementName{"Enemies"}, ElementType::Selector_Base },
			{ ElementName{"Within_Range"}, ElementType::Selector_Generic },
			{ ElementName{"One"}, ElementType::Number }
		};
		TestASTFromStream(stream);

		stream = {
			{ ElementName{"Attack"}, ElementType::Action },
			{ ElementName{"Within_Range"}, ElementType::Selector_Generic },
			{ ElementName{"Enemies"}, ElementType::Selector_Base }
		};

		TestASTFromStream(stream);
		*/

		return true;
	}
};


#endif // TEST_AST_PARSING_HPP
