#ifndef TEST_AST_PARSING_HPP
#define TEST_AST_PARSING_HPP

#include <assert.h> 

#include "../../../farb/tests/RegisterTest.hpp"
#include "../../src/command/ElementTypes.cpp"


using namespace Farb;

using namespace Farb::Tests;

using namespace Command;

void TestASTFromStream(
	std::vector<ElementToken> & tokens,
	ElementNode & expected_result,
	bool complete)
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
	bool success = ElementNode::Equal(expected_result, *parser.root);

	std::string printString = ElementNode::GetPrintString(*parser.root, "            ");
	
	farb_print(success,
		"parsed ast from stream: [ " + stream + "]\n" + printString);
	
	assert(success);
}

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

		ElementNode expected { stream[0], ElementIndex{0} };
		ElementNode implied = ElementNode{
			ImpliedNodeOptions::selectorToken, kNullElementIndex};
		implied.children.push_back(ElementNode{stream[1], ElementIndex{1}});
		implied.childArgumentMapping.insert({ ParameterIndex{0}, ElementIndex{0} });
		expected.children.push_back(implied);
		expected.childArgumentMapping.insert({ ParameterIndex{0}, ElementIndex{0} });

		TestASTFromStream(stream, expected, true);

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
