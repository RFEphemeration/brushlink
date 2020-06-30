#include "Global_Functions.h"

namespace Command
{

ErrorOr<Number> NumberFromDigits::Evaluate(Context & context, std::vector<Digit> digits)
{
	int value = 0;
	for (auto & digit : digits)
	{
		value = (value * 10) + digit.value;
	}
	return Number{value};
}

std::string NumberFromDigits::Print(const GlobalFunction & element, std::string line_prefix)
{
	// @Cleanup this is an invasive function, is that okay?
	std::string print_string = line_prefix;
	for (auto & arg : element.parameters[0]->arguments)
	{
		auto * digit = dynamic_cast<Literal<Digit> * >(arg.get());
		if (digit == nullptr)
		{
			Error("NumberLiteral should only have Literal<Digit> arguments");
		}
		print_string += str(digit->value);
	}
}

} // namespace Command
