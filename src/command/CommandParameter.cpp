#include "CommandParameter.hpp"
#include "CommandElement.hpp"


namespace Command
{

value_ptr<CommandParameter> Param(
	ElementType::Enum type,
	ElementName default_value,
	OccurrenceFlags::Enum flags)
{

	if (!(flags & OccurrenceFlags::Optional)
		&& !default_value.value.empty())
	{
		Error("Param is required but has a default value of " + default_value.value).Log();
	}

	if (flags == static_cast<OccurrenceFlags::Enum>(0x0))
	{
		return new ParamSingleRequired(type);
	}
	else if (flags == OccurrenceFlags::Optional)
	{
		return new ParamSingleOptional(type, default_value);
	}
	else if (flags == OccurrenceFlags::Repeatable)
	{
		return new ParamRepeatableRequired(type);
	}
	else if (flags == (OccurrenceFlags::Repeatable | OccurrenceFlags::Optional))
	{
		return new ParamRepeatableOptional(type, default_value);
	}
	Error("Param flags are not supported " + std::to_string((int)flags));
	return nullptr;
}

ErrorOr<Success> CommandParameter::SetArgument(CommandElement * argument)
{
	if (argument == nullptr)
	{
		return Error("Argument is null");
	}
	if (GetAllowedTypes().count(argument->Type()) == 0)
	{
		return Error("Argument is of unacceptable type");
	}
	return SetArgumentInternal(argument);
}

std::string ParamSingleRequired::GetPrintString(std::string line_prefix)
{
	if (argument != nullptr)
	{
		return argument->GetPrintString(line_prefix);
	}
	else
	{
		return "";
	}
}

bool ParamSingleRequired::IsSatisfied()
{
	return argument != nullptr
	&& argument->ParametersSatisfied();
}

ErrorOr<Success> ParamSingleRequired::SetArgumentInternal(CommandElement * argument)
{
	if (argument != nullptr)
	{
		return Error("Cannot accept multiple arguments for this parameter");
	}
	this->argument = argument;
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


std::string ParamSingleOptional::GetPrintString(std::string line_prefix)
{
	if (argument != nullptr)
	{
		return argument->GetPrintString(line_prefix);
	}
	else if (!default_value.value.empty())
	{
		return line_prefix + "(" + default_value.value + ")\n";
	}
	else
	{
		return "";
	}
}

bool ParamSingleOptional::IsSatisfied()
{
	return argument == nullptr || argument->ParametersSatisfied();
}

ErrorOr<Value> ParamSingleOptional::Evaluate(CommandContext & context)
{
	if (argument == nullptr)
	{
		if (default_value.value.empty())
		{
			return Error("Optional param with no default should not be Evaluated");
		}
		// todo: get value for default
	}
	return argument->Evaluate(context);
}

std::string ParamRepeatableRequired::GetPrintString(std::string line_prefix)
{
	std::string print_string = "";
	for (auto arg : arguments)
	{
		print_string += arg->GetPrintString(line_prefix);
	}
	return print_string;
}

bool ParamRepeatableRequired::IsSatisfied()
{
	return !arguments.empty()
		&& arguments[arguments.size()-1]->ParametersSatisfied();
}


// todo: should this be passed in as a unique_ptr?
ErrorOr<Success> ParamRepeatableRequired::SetArgumentInternal(CommandElement * argument)
{
	this->arguments.emplace_back(argument);
	return Success();
}

CommandElement * ParamRepeatableRequired::GetLastArgument()
{
	if (arguments.size() > 0)
	{
		return arguments[arguments.size() - 1].get();
	}
	return nullptr;
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

ErrorOr<std::vector<Value> > ParamRepeatableRequired::EvaluateRepeatable(CommandContext & context)
{
	std::vector<Value> values;

	for (auto argument : arguments)
	{
		values.push_back(CHECK_RETURN(argument->Evaluate(context)));
	}
	return values;
}

std::string ParamRepeatableOptional::GetPrintString(std::string line_prefix)
{
	std::string print_string = "";
	for (auto arg : arguments)
	{
		print_string += arg->GetPrintString(line_prefix);
	}
	return print_string;
	
	if (arguments.empty() && !default_value.value.empty())
	{
		print_string += line_prefix + "(" + default_value.value + ")\n";
	}

	return print_string;
}

bool ParamRepeatableOptional::IsSatisfied()
{
	return arguments.empty()
		|| arguments[arguments.size()-1]->ParametersSatisfied();
}


ErrorOr<Value> ParamRepeatableOptional::Evaluate(CommandContext & context)
{
	if (arguments.size() > 1)
	{
		return Error("There are too many arguments here for evaluating as a single");
	}
	if (arguments.size() == 0)
	{
		// @Incomplete: get default value
	}
	return arguments[0]->Evaluate(context);
}

ErrorOr<std::vector<Value> > ParamRepeatableOptional::EvaluateRepeatable(CommandContext & context)
{
	std::vector<Value> values;

	for (auto argument : arguments)
	{
		values.push_back(CHECK_RETURN(argument->Evaluate(context)));
	}
	if (arguments.size() == 0 && !default_value.value.empty())
	{
		// @Incomplete: get default value
	}
	return values;
}

std::string OneOf::GetPrintString(std::string line_prefix)
{
	std::string print_string = "";
	if (chosen_index != -1)
	{
		print_string += possibilities[chosen_index]->GetPrintString(line_prefix);
	}
	else
	{
		for (auto && option : possibilities)
		{
			// OneOf defaults to first option with default
			if (option->IsSatisfied())
			{
				print_string += option->GetPrintString(line_prefix);
				break;
			}
		}
	}
	
	return print_string;
}

Set<ElementType::Enum> OneOf::GetAllowedTypes()
{
	if (chosen_index != -1)
	{
		return possibilities[chosen_index]->GetAllowedTypes();
	}
	Set<ElementType::Enum> allowed;
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
	if (chosen_index != -1)
	{
		return possibilities[chosen_index]->IsSatisfied();
	}
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
	for (int index = 0; index < possibilities.size(); index++)
	{
		auto parameter = possibilities[index];
		auto result = parameter->SetArgument(argument);
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

CommandElement * OneOf::GetLastArgument()
{
	if (chosen_index == -1)
	{
		return nullptr;
	}
	return possibilities[chosen_index]->GetLastArgument();
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

ErrorOr<std::vector<Value> > OneOf::EvaluateRepeatable(CommandContext & context)
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
