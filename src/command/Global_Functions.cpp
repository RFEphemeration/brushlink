#include "Global_Functions.h"

namespace Command
{

ErrorOr<Variant> KeyWords::Sequence(Context & context, std::vector<Variant> args)
{
	if (args.empty())
	{
		return Success();
	}
	return args.back();
}

ErrorOr<Variant> KeyWords::Repeat(Context & context, Number count, ValueName name, const Parameter * operation)
{
	// how do we want child contexts to work?
	// should we just polute the local namespace?
	Context child = context.MakeChild();
	for (int i = 0; i < count.value; i++)
	{
		child.SetLocal(name, Number{i});
		if (i == count.value - 1)
		{
			return operation->Evaluate(child);
		}
		else
		{
			CHECK_RETURN(operation->Evaluate(child));
		}
	}
	return Variant{Success{}};
}

ErrorOr<Variant> KeyWords::ForEach(Context & context, std::vector<Variant> args, ValueName name, const Parameter * operation)
{
	Context child = context.MakeChild();
	Variant value {Success{}};
	// @Feature Push/Pop local variable shadowing?
	for (int i = 0; i < args.size(); i++)
	{
		child.SetLocal(name, args[i]);
		value = CHECK_RETURN(operation->Evaluate(child));
	}
	return value;
}

ErrorOr<Variant> KeyWords::ForEachUnit(Context & context, Unit_Group group, ValueName name, const Parameter * operation)
{
	Context child = context.MakeChild();
	Variant value {Success{}};
	for (int i = 0; i < group.members.size(); i++)
	{
		child.SetLocal(name, group.members[i]);
		value = CHECK_RETURN(operation->Evaluate(child));
	}
	return value;
}

ErrorOr<Variant> KeyWords::ForEachPoint(Context & context, Variant set, ValueName name, const Parameter * operation)
{
	Context child = context.MakeChild();
	Variant value {Success{}};
	Variant_Type type = GetVariantType(set);
	if (type == Variant_Type::Line)
	{
		Line & line = std::get<Line>(set);
		for (int i = 0; i < line.points.size(); i++)
		{
			child.SetLocal(name, line.points[i]);
			value = CHECK_RETURN(operation->Evaluate(child));
		}
	}
	else if (type == Variant_Type::Area)
	{
		Area & area = std::get<Area>(set);
		for (int i = 0; i < area.points.size(); i++)
		{
			child.SetLocal(name, area.points[i]);
			value = CHECK_RETURN(operation->Evaluate(child));
		}
	}
	else
	{
		return Error("ForEachPoint expected a line or area");
	}
	return value;
}

ErrorOr<Variant> KeyWords::If(Context & context, Bool choice, const Parameter * primary,const Parameter * secondary)
{
	if (choice)
	{
		return primary->Evaluate(context);
	}
	else
	{
		return secondary->Evaluate(context);
	}
}

ErrorOr<Variant> KeyWords::IfError(Context & context,
	const Parameter * check,
	const Parameter * error,
	const Parameter * value)
{
	auto result = check->Evaluate(context);
	if (result.IsError())
	{
		return error->Evaluate(context);
	}
	else
	{
		return value->Evaluate(context);
	}
}

ErrorOr<Variant> KeyWords::While(Context & context, const Parameter * condition, const Parameter * operation)
{
	Bool result = true;
	Variant value {Success{}};
	// @Feature execution counting number of elements evaluated, delaying until next turn?
	// break if count > 10,000 or something large?
	while(CHECK_RETURN(condition->EvaluateAs<Bool>(context)))
	{
		value = CHECK_RETURN(operation->Evaluate(context));
	}
	return value;
}

ErrorOr<Number> NumberLiteral::Evaluate(std::vector<Digit> digits)
{
	int value = 0;
	for (auto & digit : digits)
	{
		value = (value * 10) + digit.value;
	}
	return Number{value};
}

std::string NumberLiteral::Print(const Element & element, std::string line_prefix)
{
	// @Cleanup this is an invasive function, is that okay?
	std::string print_string = line_prefix;
	for (auto & arg : element.parameters[0]->arguments)
	{
		auto * digit = dynamic_cast<Literal<Digit> * >(arg.get());
		if (digit == nullptr)
		{
			Error("NumberLiteral should only have Literal<Digit> arguments").Log();
			break;
		}
		print_string += str(digit->value);
	}
	return print_string;
}

ErrorOr<Number> NumberOperators::Add(Number a, Number b)
{
	return Number{a.value + b.value};
}

ErrorOr<Number> NumberOperators::Subtract(Number a, Number b)
{
	return Number{a.value - b.value};
}

ErrorOr<Number> NumberOperators::Multiply(Number a, Number b)
{
	return Number{a.value * b.value};
}

ErrorOr<Number> NumberOperators::Divide(Number a, Number b)
{
	return Number{a.value / b.value};
}

ErrorOr<Number> NumberOperators::Sum(std::vector<Number> operands)
{
	Number sum;
	for (auto & o : operands)
	{
		sum.value += o.value;
	}
	return sum;
}

ErrorOr<ValueName> ValueNameConstructors::Literal(std::vector<Letter> letters)
{
	return ValueName{}
}

std::string ValueNameConstructors::PrintLiteral(const Element & element, std::string line_prefix)
{
	// @Cleanup this is an invasive function, is that okay?
	std::string print_string = line_prefix;
	for (auto & arg : element.parameters[0]->arguments)
	{
		auto * letter = dynamic_cast<Literal<Letter> * >(arg.get());
		if (letter == nullptr)
		{
			Error("ValueNameLiteral should only have Literal<Letter> arguments").Log();
			break;
		}
		print_string += letter->value;
	}
	return print_string;
}

ErrorOr<ValueName> ValueNameConstructors::FromNumber(Number number)
{
	return ValueName{str(number.value)};
}

ErrorOr<ValueName> ValueNameConstructors::Concatenate(std::vector<ValueName> names)
{
	ValueName result;
	for (auto & name : names)
	{
		result.value += name.value;
	}
	return result;
}

ErrorOr<Line> LocationConstructors::LineFromPoints(std::vector<Point> points)
{
	return Line{std::move(points)};
}

ErrorOr<Direction> LocationConstructors::DirectionFromTo(Point from, Point to)
{
	return Direction{to - from};
}

ErrorOr<Area> LocationConstructors::AreaUnion(std::vector<Area> areas)
{
	Area result;
	for (const auto & area : areas)
	{
		result.UnionWith(area);
	}
	return result;
}

ErrorOr<Point> LocationConstructors::PointAtAreaCenter(Area area)
{
	if (area.points.empty())
	{
		return Error("Area is empty");
	}
	Point center{0,0};
	for (auto & point : area.points)
	{
		center += point;
	}
	center /= area.points.size();
	if (Contains(area.points, center))
	{
		return center;
	}
	Point closest = center;
	int min_cardinal_distance = -1;
	for (auto & point : area.points)
	{
		int cardinal_distance = center.CardinalDistance(point);
		if (min_cardinal_distance == -1
			|| cardinal_distance < min_cardinal_distance)
		{
			closest = point;
			min_cardinal_distance = cardinal_distance;
		}
	}
	return closest;
}

} // namespace Command
