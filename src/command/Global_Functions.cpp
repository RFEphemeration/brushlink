#include "Global_Functions.h"

namespace Command
{

/*
ErrorOr<Number> ::Evaluate(Context & context, )
{
	
}
std::string ::Print(const GlobalFunction & element, std::string line_prefix)
{
	
}
*/

ErrorOr<Number> NumberFromDigits::Evaluate(Context & context, std::vector<Digit> digits)
{
	int value = 0;
	for (auto & digit : digits)
	{
		value = (value * 10) + digit.value;
	}
	return Number{value};
}

std::string NumberFromDigits::Print(const Element & element, std::string line_prefix)
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

ErrorOr<Number> NumberOperators::Add(Context & context, Number a, Number b)
{
	return Number{a.value + b.value};
}

ErrorOr<Number> NumberOperators::Subtract(Context & context, Number a, Number b)
{
	return Number{a.value - b.value};
}

ErrorOr<Number> NumberOperators::Multiply(Context & context, Number a, Number b)
{
	return Number{a.value * b.value};
}

ErrorOr<Number> NumberOperators::Divide(Context & context, Number a, Number b)
{
	return Number{a.value / b.value};
}

ErrorOr<Number> NumberOperators::Sum(Context & context, std::vector<Number> operands)
{
	Number sum;
	for (auto & o : operands)
	{
		sum.value += o.value;
	}
	return sum;
}

} // namespace Command
