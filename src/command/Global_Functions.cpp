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

ErrorOr<Variant> KeyWords::Repeat(Context & context, Number count, ValueName name, const Element * operation)
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

ErrorOr<Variant> KeyWords::ForEach(Context & context, std::vector<Variant> args, ValueName name, const Element * operation)
{
	Context child = context.MakeChild();
	Variant value {Success{}};
	for (int i = 0; i < args.size(); i++)
	{
		child.SetLocal(name, args[i]);
		value = CHECK_RETURN(operation->Evaluate(child));
	}
	return value;
}

ErrorOr<Variant> KeyWords::ForEachUnit(Context & context, UnitGroup group, ValueName name, const Element * operation)
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

ErrorOr<Variant> KeyWords::ForEachPoint(Context & context, Variant set, ValueName name, const Element * operation)
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

ErrorOr<Variant> KeyWords::If(Context & context, Bool choice, const Element * primary,const Element * secondary)
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
	const Element * check,
	const Element * error,
	const Element * value)
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

ErrorOr<Variant> KeyWords::While(Context & context, const Element * condition, const Element * operation)
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

ErrorOr<Number> NumberLiteral::Evaluate(Context & context, std::vector<Digit> digits)
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
