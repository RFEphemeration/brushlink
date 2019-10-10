#ifndef INTERACTIVE_TEST_NEXT_TOKENS_HPP
#define INTERACTIVE_TEST_NEXT_TOKENS_HPP

#include <assert.h> 

#include "../../../farb/tests/RegisterTest.hpp"
#include "../../src/command/Parser.h"
#include "../../src/command/ElementDictionary.h"

using namespace Farb;

using namespace Farb::Tests;

using namespace Command;

class InteractiveTestNextTokens : public ITest
{
public:
	virtual bool IsInteractive() const override { return true; }

	virtual bool RunTests() const override
	{
		std::cout << "Interactive Test Next Tokens" << std::endl;

		std::string line;

		Parser parser;

		std::set<ElementName> validNextElements;

		while (true)
		{
			std::cout << ": " << std::flush;
 			std::getline(std::cin, line);
 			if (line.empty())
 			{
 				return true;
 			}
 			ElementName name{line};
 			auto decl = ElementDictionary::GetDeclaration({line});
 			if (decl == nullptr)
 			{
 				std::cout << "invalid element name." << std::endl;
 				continue;
 			}
 			auto result = parser.Append({name, decl->types});
 			if (result.IsError())
 			{
 				result.GetError().Log();
 				continue;
 			}
 			std::cout << "AST - " << std::endl;
 			std::string printString = ElementNode::GetPrintString(parser.root, "    ");
 			std::cout << printString;

 			auto criteria = parser.GetNextTokenCriteria();

 			ElementDictionary::GetAllowedNextElements(criteria, validNextElements);

 			if (validNextElements.size() == 0)
 			{
 				if (!parser.IsComplete())
 				{
 					std::cout << "no valid elements and the statement is still incomplete" << std::endl;
 					return false;
 				}
 				else
 				{
 					std::cout << "command is complete" << std::endl;
 					parser.Reset();
 					continue;
 				}
 			}

 			std::cout << "Valid Next - ";
 			for (auto & nextName : validNextElements)
 			{
 				std::cout << nextName.value << " ";
 			}
 			std::cout << std::endl;
 		}
	}
};

#endif // INTERACTIVE_TEST_NEXT_TOKENS_HPP`