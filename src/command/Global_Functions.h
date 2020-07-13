#ifndef BRUSHLINK_GLOBAL_FUNCTIONS_H
#define BRUSHLINK_GLOBAL_FUNCTIONS_H

namespace Command
{

// arguments can be received as value_ptr<Element> in order to allow repeat evaluation
// const Element * is to prevent copying
// or prevent premature evaluation
namespace KeyWords
{
	ErrorOr<Variant> Sequence(Context & context, std::vector<Variant> args);
	ErrorOr<Variant> Repeat(Context & context, Number count, ValueName name, const Element * operation);

	ErrorOr<Variant> ForEach(Context & context, std::vector<Variant> args, ValueName name, const Element * operation);
	ErrorOr<Variant> ForEachUnit(Context & context, UnitGroup group, ValueName name, const Element * operation);
	ErrorOr<Variant> ForEachPoint(Context & context, Variant set, ValueName name, const Element * operation);

	ErrorOr<Variant> If(Context & context, Bool choice, const Element * primary, const Element * secondary);
	ErrorOr<Variant> While(Context & context, const Element * condition, const Element * operation);

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
