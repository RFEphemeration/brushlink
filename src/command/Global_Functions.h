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
	ErrorOr<T> CastTo(Variant value)
	{
		if (!std::holds_alternative<T>(value))
			return Error("Type mismatch during cast");
		return std::get<T>(value);
	}
}

namespace NumberLiteral
{
	ErrorOr<Number> Evaluate(std::vector<Digit> digits);
	std::string Print(const Element & element, std::string line_prefix);
};

namespace NumberOperators
{
	ErrorOr<Number> Add(Number a, Number b);
	ErrorOr<Number> Subtract(Number a, Number b);
	ErrorOr<Number> Multiply(Number a, Number b);
	ErrorOr<Number> Divide(Number a, Number b);
	ErrorOr<Number> Sum(std::vector<Number> operands);
};

namespace ValueNameConstructors
{
	ErrorOr<ValueName> Literal(std::vector<Letter> letters);
	std::string PrintLiteral(const Element & element, std::string line_prefix);
	ErrorOr<ValueName> FromNumber(Number number);
	ErrorOr<ValueName> Concatenate(std::vector<ValueName> names);
};

namespace LocationConstructors
{
	ErrorOr<Line> LineFromPoints(std::vector<Point> points);
	ErrorOr<Direction> DirectionFromTo(Point from, Point to);
	ErrorOr<Area> AreaUnion(std::vector<Area> areas);
	ErrorOr<Point> PointAtAreaCenter(Area area);
};


} // namespace Command

#endif // BRUSHLINK_GLOBAL_FUNCTIONS_H
