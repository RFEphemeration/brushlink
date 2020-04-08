#include "CommandParameter.hpp"
#include "CommandElement.hpp"


namespace Command
{

ErrorOr<Success> CommandParameter::SetArgument(CommandElement * argument)
{
	if (argument == nullptr)
	{
		return Error("Argument is null");
	}
	if (!GetAllowedTypes().contains(argument->GetType()))
	{
		return Error("Argument is of unacceptable type");
	}
	return SetArgumentImplementation(argument);
}

ErrorOr<Success> ParamSingleRequired::SetArgumentInternal(CommandElement * argument)
{
	if (argument != nullptr)
	{
		return Error("Cannot accept multiple arguments for this parameter");
	}
	this.argument = argument;
	return Success();
}

ErrorOr<Value> ParamSingleRequired::Evaluate(CommandContext & context)
{
	if (argument == nullptr)
	{
		return Error("Parameter argument is missing, cannot evaluate");
	}
	return argument->Evaluate(context);
}


ErrorOr<Value> ParamSingleOptional::Evaluate(CommandContext & context)
{
	if (argument == nullptr)
	{
		// todo: get value for default
	}
	return argument->Evaluate(context);
}

// todo: should this be passed in as a unique_ptr?
ErrorOr<Success> ParamRepeatableRequired::SetArgumentInternal(CommandElement * argument)
{
	this.arguments.append(argument);
	return Success();
}

ErrorOr<Value> ParamRepeatableRequired::Evaluate(CommandContext & context)
{
	if (arguments.size() > 1)
	{
		return Error("There are too many arguments here for evaluating as a single");
	}
	if (arguments.size() == 0)
	{
		return Error("Parameter argument is missing, cannot evaluate");	
	}
	return arguments[0]->Evaluate(context);
}

ErrorOr<Repeatable<Value> > ParamRepeatableRequired::EvaluateRepeatable(CommandContext & context)
{
	Repeatable<Value> values;

	for (auto argument : arguments)
	{
		values.append(CHECK_RETURN(argument->Evaluate(context)));
	}
	return values;
}


ErrorOr<Value> ParamRepeatableOptional::Evaluate(CommandContext & context)
{
	if (arguments.size() > 1)
	{
		return Error("There are too many arguments here for evaluating as a single");
	}
	if (arguments.size() == 0)
	{
		// todo: get default value;
	}
	return arguments[0]->Evaluate(context);
}

ErrorOr<Repeatable<Value> > ParamRepeatableOptional::EvaluateRepeatable(CommandContext & context)
{
	Repeatable<Value> values;

	for (auto argument : arguments)
	{
		values.append(CHECK_RETURN(argument->Evaluate(context)));
	}
	return values;
}

std::set<ElementType> OneOf::GetAllowedTypes()
{
	if (chosen_index != -1)
	{
		return possibilities[chosen_index]->GetAllowedTypes();
	}
	std::set<ElementType> allowed;
	for (auto parameter : possibilities)
	{
		allowed.merge(parameter->GetAllowedTypes());
	}
	return allowed;
}

bool OneOf::IsRequired()
{
	for (auto parameter : possibilities)
	{
		if (! parameter->IsRequired())
		{
			return false;
		}
	}
	return true;
}

bool OneOf::IsSatisfied()
{
	for (auto parameter : possibilities)
	{
		if (parameter->IsSatisfied())
		{
			return true;
		}
	}
	return false;
}

ErrorOr<Success> OneOf::SetArgumentInternal(CommandElement * argument)
{
	if (chosen_index != -1)
	{
		return possibilities[chosen_index]->SetArgument(argument);
	}
	else
	{
		for (int index = 0; index < possibilities.size(); index++)
		{
			auto parameter = possibilities[index];
			auto result = parameter->SetArgument(argument)
			if (!result.IsError())
			{
				chosen_index = index;
				break;
			}
		}
		if (chosen_index == -1)
		{
			return Error("Could not find a valid parameter for argument in OneOf");
		}
		return Success();
	}
}

ErrorOr<Value> OneOf::Evaluate(CommandContext & context)
{
	if (chosen_index == -1)
	{
		for (auto parameter : possibilities)
		{
			if (parameter->IsSatisfied())
			{
				return parameter->Evaluate(context);
			}
		}
		return Error("No parameter in OneOf has a default value");
	}
	else
	{
		return possibilities[chosen_index]->Evaluate(context);
	}
}

ErrorOr<Repeatable<Value> > OneOf::EvaluateRepeatable(CommandContext & context)
{
	if (chosen_index == -1)
	{
		for (auto parameter : possibilities)
		{
			if (parameter->IsSatisfied())
			{
				return parameter->EvaluateRepeatable(context);
			}
		}
		return Error("No parameter in OneOf has a default value");
	}
	else
	{
		return possibilities[chosen_index]->EvaluateRepeatable(context);
	}
}

} // namespace Command
