#ifndef TEST_AST_PARSING_HPP
#define TEST_AST_PARSING_HPP

#include <assert.h> 

#include "../../../farb/tests/RegisterTest.hpp"
#include "../../src/command/ElementTypes.hpp"
#include "../../src/command/Parser.h"

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
	for (auto pair : node.children)
	{
		ConvertToLEN(pair.second, pair.first, nestingLevel, linear);
	}
}

void TestASTFromStream(
	const std::vector<ElementName> & names,
	const std::vector<LEN> & expected_linear,
	bool complete = true)
{
	Parser parser;
	std::string stream = "";

	for (auto name : names)
	{
		auto result = parser.Append({name});
		if (result.IsError())
		{
			farb_print(false, "TestAST appending token " + name.value + " failed");
			assert(false);
		};
		stream += name.value + ", ";
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

using Stream = std::vector<ElementToken>;

class TestASTParsing : public ITest
{
public:
	virtual bool RunTests() const override
	{
		std::cout << "AST Parsing" << std::endl;

		TestASTFromStream(
			{
				{"Attack"},
				{"Enemies"}
			},
			{
				{ {"Attack"}, kNullParameterIndex, 0 },
				{ {"Selector"}, 0, 1, {true} },
				{ {"Enemies"}, 0, 2 }
			}
		);

		TestASTFromStream(
			{
				"Attack",
				"Enemies",
				"Within_Range"
			},
			{
				{ "Attack", kNullParameterIndex, 0 },
				{ "Selector", 0, 1, {true} },
				{ "Enemies", 0, 2 },
				{ "Within_Range", 2, 2}
			}
		);

		/*
		stream = {
			{ ElementName{"Attack"}, ElementType::Action },
			{ ElementName{"Enemies"}, ElementType::Set },
			{ ElementName{"Within_Range"}, ElementType::Filter }
		};

		TestASTFromStream(stream);

		stream = {
			{ ElementName{"Attack"}, ElementType::Action },
			{ ElementName{"Enemies"}, ElementType::Set },
			{ ElementName{"Within_Range"}, ElementType::Filter },
			{ ElementName{"One"}, ElementType::Number }
		};
		TestASTFromStream(stream);

		stream = {
			{ ElementName{"Attack"}, ElementType::Action },
			{ ElementName{"Within_Range"}, ElementType::Filter },
			{ ElementName{"Enemies"}, ElementType::Set }
		};

		TestASTFromStream(stream);
		*/

		return true;
	}
};


#endif // TEST_AST_PARSING_HPP
