#ifndef BRUSHLINK_GLOBAL_FUNCTIONS_H
#define BRUSHLINK_GLOBAL_FUNCTIONS_H

namespace Command
{

struct NumberFromDigits
{
	static ErrorOr<Number> Evaluate(Context & context, std::vector<Digit> digits);
	static std::string Print(const Element & element, std::string line_prefix);
};

struct NumberOperators
{
	static ErrorOr<Number> Add(Context & context, Number a, Number b);
	static ErrorOr<Number> Subtract(Context & context, Number a, Number b);
	static ErrorOr<Number> Multiply(Context & context, Number a, Number b);
	static ErrorOr<Number> Divide(Context & context, Number a, Number b);
	static ErrorOr<Number> Sum(Context & context, std::vector<Number> operands);
};


} // namespace Command

#endif // BRUSHLINK_GLOBAL_FUNCTIONS_H
