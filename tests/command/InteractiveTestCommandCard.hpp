#ifndef INTERACTIVE_TEST_COMMAND_CARD_HPP
#define INTERACTIVE_TEST_COMMAND_CARD_HPP

#include <assert.h> 

#include "../../../farb/tests/RegisterTest.hpp"
#include "../../src/command/CommandContext.h"
#include "../../src/command/CommandCard.h"
#include "../../src/command/CommandElement.hpp"
#include "../../../farb/src/utils/StringExtensions.hpp"

using namespace Farb;

using namespace Farb::Tests;

using namespace Command;

class InteractiveTestCommandCard : public ITest
{
public:
	virtual bool IsInteractive() const override { return true; }

	virtual bool RunTests() const override
	{
		std::cout << "Interactive Test Command Card" << std::endl;

		std::string line;

		CommandContext context;
		context.InitElementDictionary();
		context.InitNewCommand();

		CommandCard card{context};

		while (true)
		{
			std::cout << context.command->GetPrintString("");
			std::cout << card.MakeCurrentTabPrintString();

			std::cout << ": " << std::flush;
			std::getline(std::cin, line);
			if (line.empty())
			{
				return true;
			}
			auto words = Split(line, ' ');
			for (auto word : words)
			{
				auto result_input = card.HandleInput(word);
				if (result_input.IsError())
				{
					result_input.GetError().Log();
					break;
				}
			}
		}
	}
};

#endif // INTERACTIVE_TEST_COMMAND_CARD_HPP