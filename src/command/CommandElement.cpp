#include "CommandElement.hpp"

namespace Command
{

Map<ElementType, int> CommandElement::GetAllowedArgumentTypes()
{
	Map<ElementType, int> allowed;
	int last_param_with_args = -1;
	bool allowed_next = false;

	// find the last parameter that has arguments.
	// we can't go backwards (for now, until we implement permutable)
	// perhaps a way to implement permutable parameters would be to
	// move the first parameter in a permutable sequence to the front
	// that sounds not too hard to do and maintains the structure here
	// @Incomplete: permutable parameters
	// we could even use this to allow users to set the parameter order
	// perhaps trivially as a new word w/ the same name and swapped params
	for (int index = parameters.size() - 1; index >= 0 ; index--)
	{
		// earlier arguments to parameters can't be revisited
		CommandElement * argument = parameters[index].GetLastArgument();
		if (argument != nullptr)
		{
			last_param_with_args = index;
			for (auto pair : argument->GetAllowedArgumentTypes())
			{
				if (allowed.contains(pair.key))
				{
					allowed[pair.key] += pair.value;
				}
				else
				{
					allowed[pair.key] = pair.value;
				}
			}
			if (argument->IsSatisfied())
			{
				allowed_next = true;
			}
			break;
		}
	}
	if (!allowed_next)
	{
		return allowed;
	}

	for (int index = last_param_with_args + 1; index < parameters.size(); index++)
	{
		
		// there are no arguments in any of these parameters
		// so we just ask the parameter directly
		for (auto type : parameters[index].GetAllowedTypes())
		{
			if (allowed.contains(type))
			{
				allowed[pair.key] += 1;
			}
			else
			{
				allowed[pair.key] = 1;
			}
		}
		// @Incomplete permutable
		if (parameters[index].IsRequired())
		{
			break;
		}
	}

	return allowed;
}

Set<ElementType> CommandElement::ParameterAllowedTypes(int index)
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
