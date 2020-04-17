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
			if (argument->ParametersSatisfied())
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
		for (auto type : parameters[index]->GetAllowedTypes())
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
		if (parameters[index]->IsRequired())
		{
			break;
		}
	}

	return allowed;
}

ErrorOr<bool> CommandElement::AppendArgument(std::unique_ptr<CommandElement> * next, int & skip_count)
{
	int last_param_with_args = -1;
	for (int index = parameters.size() - 1; index >= 0 ; index--)
	{
		// earlier arguments to parameters can't be revisited
		CommandElement * argument = parameters[index]->GetLastArgument();
		if (argument != nullptr)
		{
			last_param_with_args = index;
			bool result = CHECK_RETURN(argument->AppendArgument(next, skip_count));
			if (result)
			{
				return true;
			}
			/*
			// this check should already be handled by calls down the chain with the
			// error message below
			if (!argument->ParametersSatisfied())
			{
				return Error("Can't append because a preceding element has not had all parameters satisfied");
			}
			*/
		}
	}

	for (int index = last_param_with_args+1; index < parameters.size(); index++)
	{
		auto types = parameters[index]->GetAllowedTypes();
		if (types.contains(next->Type()))
		{
			if (skip_count == 0)
			{
				parameters[index]->SetArgument(next);
				return true;
			}
			skip_count--;
		}
		// @Incomplete permutable
		if (parameters[index]->IsRequired())
		{
			return Error("Can't append because a preceding parameter has not been satisfied");
		}
	}
	return false;
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
	if (!parameters[index]->GetAllowedTypes().contains(argument->Type()))
	{
		return false;
	}

	parameters[index]->SetArgument(argument);
}

bool CommandElement::ParametersSatisfied()
{
	for (auto parameter : parameters)
	{
		if (!parameter->IsSatisfied())
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



std::unique_ptr<CommandElement> EmptyCommandElement::DeepCopy()
{
	std::vector<std::unique_ptr<CommandParameter> > params_copy;
	for (auto param : parameters)
	{
		params_copy.append(param->DeepCopy());
	}
	auto * copy = new ContextFunctionWithActors(type, func, params_copy);
	return copy;
}

ErrorOr<Value> EmptyCommandElement::Evaluate(CommandContext & context)
{
	if (left_parameter != nullptr)
	{
		CHECK_RETURN(left_parameter->Evaluate(context));
	}
	for (auto && param : parameters)
	{
		CHECK_RETURN(param->Evaluate(context))
	}
	return Success();
}

std::unique_ptr<CommandElement> SelectorCommandElement::DeepCopy()
{
	std::vector<std::unique_ptr<CommandParameter> > params_copy;
	for (auto param : parameters)
	{
		params_copy.append(param->DeepCopy());
	}
	return new SelectorCommandElement(params_copy);
}

ErrorOr<Value> SelectorCommandElement::Evaluate(CommandContext & context)
{
	// an alternative format for this could be to allow elements
	// to have private parameters that don't return in the get allowed types
	// and don't prevent the user from moving on
	// but then would throw errors during evaluation

	// context dependent defaults should happen here
	// maybe CommandElements should have a pointer to their parent?
	// we had considered that for access to actors and everything
	
	UnitGroup units = CHECK_RETURN(parameters[0]->EvaluateAs<UnitGroup>(context));
	Repeatable<Filter> filters = CHECK_RETURN(parameters[1]->EvaluateAsRepeatable<Filter>(context));

	for (auto filter : filters)
	{
		units = filter(units);
	}

	int count = 0;
	if (parameters[2]->GetLastArgument() != nullptr)
	{
		// group size is optional and doesn't have a default
		// so we can only evaluate it if it has an argument
		GroupSize size = CHECK_RETURN(parameters[2]->EvaluateAs<Number>(context));
		count = size(units);
	}
	else if (parameters[3]->GetLastArgument() != nullptr)
	{
		// if GroupSize is not set but Superlative is, target a single unit
		count = 1;
	}
	else
	{
		// default, with no GroupSize or Superlative set, just take all the units
		count = units.members.size();
	}

	// only apply superlative if we have too many units
	if (units.members.size() > count)
	{
		// superlatives should have a default value, so even if there isn't 
		// a last argument, we should be able to evaluate
		Superlative superlative = CHECK_RETURN(parameters[3]->EvaluateAs<Superlative>(context));

		units = superlative(units, Number(count));
	}
	
	return units;
}

} // namespace Command
