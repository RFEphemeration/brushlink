#include "CommandElement.hpp"

namespace Command
{



std::set<ElementType> CommandElement::ParameterAllowedTypes(int index)
{
	if (index >= ParameterCount())
	{
		return {};
	}
	return parameters[index]->GetAllowedTypes();
}

bool CommandElement::AddArgument(int index, CommandElement * argument)
{
	if (index >= ParameterCount())
	{
		return false;
	}
	if (argument == nullptr)
	{
		return false;
	}
	if (!parameters[index].GetAllowedTypes().contains(argument->Type()))
	{
		return false;
	}

	parameters[index].SetArgument(argument);
}

bool CommandElement::ParametersSatisfied()
{
	for (auto parameter : parameters)
	{
		if (!parameter.IsSatisfied())
		{
			return false;
		}
	}
	return true;
}

ErrorOr<Value> Select::Evaluate(CommandContext & context)
{
	UnitGroup actor_units = CHECK_RETURN(parameters[0]->EvaluateAs<UnitGroup>(context));
	// this shouldn't really do anything because there are no other parameters to evaluate
	// but oh well
	context.PushActors(actor_units);
	auto result = context.Select(actor_units);
	context.PopActors();
	return result;
}

ErrorOr<Value> Move::Evaluate(CommandContext & context)
{
	UnitGroup actor_units = CHECK_RETURN(parameters[0]->EvaluateAs<UnitGroup>(context));
	// must set actors before evaluating future parameters
	// because they could use the actors as part of their evaluation
	// what if this is part of a nested call?
	// maybe whenever an action is taken the actors stack is popped?
	context.PushActors(actor_units);
	Location target = CHECK_RETURN(parameters[1]->EvaluateAs<Location>(context));
	auto result = context.Move(actor_units, target);
	context.PopActors();
	return result;
}

ErrorOr<Value> CurrentSelection::Evaluate(CommandContext & context)
{
	return context.CurrentSelection();
}

} // namespace Command
