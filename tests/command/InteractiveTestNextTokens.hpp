#ifndef INTERACTIVE_TEST_NEXT_TOKENS_HPP
#define INTERACTIVE_TEST_NEXT_TOKENS_HPP

#include <assert.h> 

#include "../../../farb/tests/RegisterTest.hpp"
#include "../../src/command/CommandContext.h"
#include "../../src/command/CommandElement.hpp"
#include "../../../farb/src/utils/StringExtensions.hpp"

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
		std::cout << "element dictionary size: " << context.element_dictionary.size() << std::endl;
		context.InitNewCommand();
		std::cout << "allowed types size: " << context.allowed_next_elements.size() << std::endl;

		for (auto pair : context.allowed_next_elements)
		{
			std::cout << "allowed " << ToString(pair.first) << " " << pair.second << std::endl;
		}

		std::cout << "AST - " << std::endl;
		std::cout << context.command->GetPrintString("    ");

		Set<ElementName> validNextElements;

		while (true)
		{
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
			auto words = Split(line, ' ');
			for (auto word : words)
			{
				auto result_token = context.GetTokenForName(ElementName{word});
				if (result_token.IsError())
				{
					result_token.GetError().Log();
					break;
				}

				auto result_handle = context.HandleToken(result_token.GetValue());
				if (result_handle.IsError())
				{
					result_handle.GetError().Log();
					break;
				}
			}

			
			std::cout << "AST - " << std::endl;
			std::string printString = context.command->GetPrintString("    ");
			std::cout << printString;
		}
	}
};

#endif // INTERACTIVE_TEST_NEXT_TOKENS_HPP`