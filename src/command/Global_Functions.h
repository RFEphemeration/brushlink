#ifndef BRUSHLINK_GLOBAL_FUNCTIONS_H
#define BRUSHLINK_GLOBAL_FUNCTIONS_H

namespace Command
{

// const Parameter * allows repeat evaluation
// or to conditionally not evaluate a parameter
// but it doesn't distinguish between repeatable or not...
namespace KeyWords
{
	ErrorOr<Variant> Sequence(Context & context, std::vector<Variant> args);
	ErrorOr<Variant> Repeat(Context & context, Number count, ValueName name, const Parameter * operation);

	ErrorOr<Variant> ForEach(Context & context, std::vector<Variant> args, ValueName name, const Parameter * operation);
	ErrorOr<Variant> ForEachUnit(Context & context, UnitGroup group, ValueName name, const Parameter * operation);
	ErrorOr<Variant> ForEachPoint(Context & context, Variant set, ValueName name, const Parameter * operation);

	ErrorOr<Variant> If(Context & context, Bool choice, const Parameter * primary, const Parameter * secondary);
	ErrorOr<Variant> IfError(Context & context, const Parameter * check, const Parameter * error, const Parameter * value);
	ErrorOr<Variant> While(Context & context, const Parameter * condition, const Parameter * operation);

	template<typename T>
	ErrorOr<T> CastTo(Context & context, Variant value)
	{
		if (!std::holds_alternative<T>(value))
			return Error("Type mismatch during cast");
		return std::get<T>(value);
	}
}

namespace NumberLiteral
{
	ErrorOr<Number> Evaluate(Context & context, std::vector<Digit> digits);
	std::string Print(const Element & element, std::string line_prefix);
};

namespace NumberOperators
{
	ErrorOr<Number> Add(Context & context, Number a, Number b);
	ErrorOr<Number> Subtract(Context & context, Number a, Number b);
	ErrorOr<Number> Multiply(Context & context, Number a, Number b);
	ErrorOr<Number> Divide(Context & context, Number a, Number b);
	ErrorOr<Number> Sum(Context & context, std::vector<Number> operands);
};


} // namespace Command

#endif // BRUSHLINK_GLOBAL_FUNCTIONS_H
