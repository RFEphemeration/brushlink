#ifndef INTERACTIVE_TEST_NEXT_TOKENS_HPP
#define INTERACTIVE_TEST_NEXT_TOKENS_HPP

#include <assert.h> 

#include "../../../farb/tests/RegisterTest.hpp"
#include "../../src/command/CommandContext.h"

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

		CommandContext context;
		context.InitElementDictionary();
		context.InitNewCommand();

		std::set<ElementName> validNextElements;

		while (true)
		{

			auto criteria = parser.GetNextTokenCriteria();

			context.GetAllowedNextElements(validNextElements);

			if (validNextElements.size() == 0)
			{
				if (!context.command->ParametersSatisfied())
				{
					std::cout << "no valid elements and the statement is still incomplete" << std::endl;
					return false;
				}
				/*
				else
				{
					std::cout << "command is complete" << std::endl;
					parser.FillDefaultArguments();
					std::cout << "AST with defaults filled- " << std::endl;
		 			std::string printString = ElementNode::GetPrintString(parser.root, "    ");
					std::cout << printString;

					parser.Reset();
					continue;
				}
				*/
			}
			
			std::cout << "Valid Next - ";
			for (auto & nextName : validNextElements)
			{
				std::cout << nextName.value << " ";
			}
			std::cout << std::endl;

			std::cout << ": " << std::flush;
			std::getline(std::cin, line);
			if (line.empty())
			{
				return true;
			}
			auto result = context.GetTokenForName(ElementName{line});
			if (result.IsError())
			{
				result.GetError().Log();
				continue;
			}

			auto result = context.HandleToken(result.GetValue());
			if (result.IsError())
			{
				result.GetError().Log()
				continue;
			}
			std::cout << "AST - " << std::endl;
			std::string printString = context.command->GetPrintString("    ");
			std::cout << printString;
		}
	}
};

#endif // INTERACTIVE_TEST_NEXT_TOKENS_HPP`