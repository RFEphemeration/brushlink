#include "CommandElement.hpp"

namespace Command
{

Table<ElementType::Enum, int> CommandElement::GetAllowedArgumentTypes()
{
	Table<ElementType::Enum, int> allowed;
	int last_param_with_args = -1;
	bool allowed_next = true;

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
		CommandElement * argument = parameters[index]->GetLastArgument();
		if (argument != nullptr)
		{
			last_param_with_args = index;
			for (auto pair : argument->GetAllowedArgumentTypes())
			{
				if (allowed.count(pair.first) > 0)
				{
					allowed[pair.first] += pair.second;
				}
				else
				{
					allowed[pair.first] = pair.second;
				}
			}
			if (!argument->ParametersSatisfied())
			{
				allowed_next = false;
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
			if (allowed.count(type) > 0)
			{
				allowed[type] += 1;
			}
			else
			{
				allowed[type] = 1;
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

ErrorOr<bool> CommandElement::AppendArgument(value_ptr<CommandElement>&& next, int & skip_count)
{
	int last_param_with_args = -1;
	for (int index = parameters.size() - 1; index >= 0 ; index--)
	{
		// earlier arguments to parameters can't be revisited
		CommandElement * argument = parameters[index]->GetLastArgument();
		if (argument != nullptr)
		{
			last_param_with_args = index;
			bool result = CHECK_RETURN(argument->AppendArgument(std::move(next), skip_count));
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
		if (types.count(next->Type()) > 0)
		{
			if (skip_count == 0)
			{
				CHECK_RETURN(parameters[index]->SetArgument(std::move(next)));
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

Set<ElementType::Enum> CommandElement::ParameterAllowedTypes(int index)
{
	if (index >= ParameterCount())
	{
		return {};
	}
	return parameters[index]->GetAllowedTypes();
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

std::string CommandElement::GetPrintString(std::string line_prefix)
{
	std::string print_string = line_prefix + name.value + "\n";
	line_prefix += "    ";
	for (auto & parameter : parameters)
	{
		print_string += parameter->GetPrintString(line_prefix);
	}
	return print_string;
}

ErrorOr<Value> EmptyCommandElement::Evaluate(CommandContext & context)
{
	if (left_parameter != nullptr)
	{
		CHECK_RETURN(left_parameter->Evaluate(context));
	}
	for (auto && param : parameters)
	{
		CHECK_RETURN(param->Evaluate(context));
	}
	return Value{Success()};
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
	std::vector<Filter> filters = CHECK_RETURN(parameters[1]->EvaluateAsRepeatable<Filter>(context));

	for (auto filter : filters)
	{
		units = filter(units);
	}

	int count = 0;
	if (parameters[2]->GetLastArgument() != nullptr)
	{
		// group size is optional and doesn't have a default
		// so we can only evaluate it if it has an argument
		GroupSize size = CHECK_RETURN(parameters[2]->EvaluateAs<GroupSize>(context));
		count = size(units).value;
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
	
	return Value{units};
}

} // namespace Command
