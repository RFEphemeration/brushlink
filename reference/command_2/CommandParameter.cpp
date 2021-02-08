#include "CommandParameter.hpp"

#include "ContainerExtensions.hpp"

#include "CommandElement.hpp"

namespace Command
{

value_ptr<CommandParameter> Param(
	CommandContext& context,
	ElementType::Enum type,
	ElementName default_value,
	OccurrenceFlags::Enum flags)
{

	if (!(flags & (OccurrenceFlags::Optional | OccurrenceFlags::Implied) )
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
	else if (flags == OccurrenceFlags::Implied)
	{
		// @Feature if we want to allow for recursive words then this can't happen
		// at param creation and needs to be a separate step
		auto * param = new ParamSingleRequired(type);
		auto result = context.GetNewCommandElement(default_value.value);
		if (result.IsError())
		{
			Error("Param with implied argument " + default_value.value + " that we could not get", new Error(result.GetError())).Log();
			return nullptr;
		}
		result.GetValue()->implicit = Implicit::Child;
		param->SetArgument(context, std::move(result.GetValue()));
		return param;
	}
	Error("Param flags are not supported " + std::to_string((int)flags));
	return nullptr;
}

value_ptr<CommandParameter> ParamImplied(
	CommandContext& context,
	value_ptr<CommandElement>&& default_value)
{
	auto * param = new ParamSingleRequired(default_value->Type());
	default_value->implicit = Implicit::Child;
	param->SetArgument(context, std::move(default_value));
	return param;
}

ErrorOr<Success> CommandParameter::SetArgument(CommandContext & context, value_ptr<CommandElement>&& argument)
{
	if (argument == nullptr)
	{
		return Error("Argument is null");
	}
	if (!Contains(GetAllowedTypes(), argument->Type()))
	{
		return Error("Argument is of unacceptable type");
	}
	return SetArgumentInternal(context, std::move(argument));
}

std::string ParamSingleRequired::GetPrintString(std::string line_prefix) const
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

bool ParamSingleRequired::IsSatisfied() const
{
	return argument != nullptr
	&& argument->ParametersSatisfied();
}

bool ParamSingleRequired::HasExplicitArgOrChild() const
{
	return argument != nullptr && argument->IsExplicitOrHasExplicitChild();
}

bool ParamSingleRequired::RemoveLastArgument()
{
	bool removed = argument != nullptr;
	argument = nullptr;
	return removed;
}

ErrorOr<Success> ParamSingleRequired::SetArgumentInternal(CommandContext & context, value_ptr<CommandElement>&& argument)
{
	if (this->argument != nullptr)
	{
		return Error("Cannot accept multiple arguments for this parameter");
	}
	this->argument = argument;
	this->argument->location_in_parent = &(this->argument);
	return Success();
}

ErrorOr<Value> ParamSingleRequired::Evaluate(CommandContext & context) const
{
	if (argument == nullptr)
	{
		return Error("Parameter argument is missing, cannot evaluate");
	}
	return argument->Evaluate(context);
}


std::string ParamSingleOptional::GetPrintString(std::string line_prefix) const
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

bool ParamSingleOptional::IsSatisfied() const
{
	return argument == nullptr || argument->ParametersSatisfied();
}

ErrorOr<Value> ParamSingleOptional::Evaluate(CommandContext & context) const
{
	if (argument == nullptr)
	{
		if (default_value.value.empty())
		{
			return Error("Optional param with no default should not be Evaluated");
		}
		else
		{
			auto default_element = CHECK_RETURN(context.GetNewCommandElement(default_value.value));
			return default_element->Evaluate(context);
		}
	}
	return argument->Evaluate(context);
}

ParamSingleImpliedOptions::ParamSingleImpliedOptions(
	ElementType::Enum type,
	std::vector<value_ptr<CommandElement>>&& implied_options)
	: ParamSingleRequired(type)
	, implied_options(implied_options)
{
	for (auto & option : this->implied_options)
	{
		if (option->Type() != type)
		{
			Error("ParamSingleImpliedOptions options must match type").Log();
		}
		// these are considered an implicit parent
		// because they're not truly enabled until a child is added
		// and Implicit::Parent vs Child controls when to remove after Undo
		option->implicit = Implicit::Parent;
	}
}

std::string ParamSingleImpliedOptions::GetPrintString(std::string line_prefix) const
{
	if (argument != nullptr)
	{
		return ParamSingleRequired::GetPrintString(line_prefix);
	}
	std::string lines = "";
	bool first = true;
	for (auto & option : implied_options)
	{
		if (first)
		{
			lines += option->GetPrintString(line_prefix + "+");
			first = false;
		}
		else
		{
			lines += option->GetPrintString(line_prefix + "|");
		}
	}
	return lines;
}

std::vector<ElementType::Enum> ParamSingleImpliedOptions::GetAllowedTypes() const
{
	if (argument != nullptr)
	{
		return {};
	}
	Set<ElementType::Enum> allowed_set = {type};
	std::vector<ElementType::Enum> allowed_list = {type};

	AllowedTypes allowed;

	// @Feature @Bug left param replacing implied option parent
	for (auto & option : implied_options)
	{
		option->GetAllowedArgumentTypes(allowed);
	}
	for (auto && allowed_type : allowed.priority)
	{
		if (allowed_type.is_left
			|| Contains(allowed_set, allowed_type.type))
		{
			continue;
		}
		allowed_set.insert(allowed_type.type);
		allowed_list.push_back(allowed_type.type);
	}
	return allowed_list;
}

bool ParamSingleImpliedOptions::IsRequired() const
{
	for (auto & option : implied_options)
	{
		if (option->ParametersSatisfied())
		{
			return false;
		}
	}
	return true;
}

bool ParamSingleImpliedOptions::IsSatisfied() const
{
	if (argument != nullptr)
	{
		return argument->ParametersSatisfied();
	}
	for (auto & option : implied_options)
	{
		if (option->ParametersSatisfied())
		{
			return true;
		}
	}
	return false;
}

ErrorOr<Success> ParamSingleImpliedOptions::SetArgumentInternal(CommandContext & context, value_ptr<CommandElement>&& argument)
{
	if (this->argument != nullptr)
	{
		return Error("Cannot accept multiple arguments for this parameter");
	}
	if (argument->Type() == type)
	{
		this->argument = argument;
		this->argument->location_in_parent = &(this->argument);
		return Success();
	}
	// @Incomplete @Bug LeftParams of implied options
	for (auto & option : implied_options)
	{
		AllowedTypes option_allowed;
		option->GetAllowedArgumentTypes(option_allowed);
		if (option_allowed.total_right.count(argument->Type()) > 0
			&& option_allowed.total_right.at(argument->Type()) > 0)
		{
			// intentional call to copy constructor, we want to preserve
			// the original option in case we Undo adding this argument
			this->argument = option;
			this->argument->location_in_parent = &(this->argument);
			// @Bug should probably pass in skips here, once we also
			// account for them in GetAllowedArguments()
			int no_skips = 0;
			bool success = CHECK_RETURN(this->argument->AppendArgument(context, std::move(argument), no_skips));
			if (!success)
			{
				return Error("ParamSingleImpliedOptions tried too append but failed");
			}
			return Success();
		}
	}
	return Error("ParamSingleImpliedOptions could not find an option to append to");
}

ErrorOr<Value> ParamSingleImpliedOptions::Evaluate(CommandContext & context) const
{
	if (argument != nullptr)
	{
		return argument->Evaluate(context);
	}
	for (auto & option : implied_options)
	{
		if (option->ParametersSatisfied())
		{
			// Evaluate isn't const and can perform modifications
			// so we are copying it into argument, same as Optionals
			// we could consider just making a local copy
			// but then that would make evaluate const, so we wouldn't need to copy at all? that's probably for the best...
			return option->Evaluate(context);
		}
	}
	return Error("ParamSingleImpliedOptions has no argument and no complete option");
}

std::string ParamRepeatableRequired::GetPrintString(std::string line_prefix) const
{
	std::string print_string = "";
	for (auto arg : arguments)
	{
		print_string += arg->GetPrintString(line_prefix);
	}
	return print_string;
}

bool ParamRepeatableRequired::IsSatisfied() const
{
	return !arguments.empty()
		&& arguments[arguments.size()-1]->ParametersSatisfied();
}

bool ParamRepeatableRequired::HasExplicitArgOrChild() const
{
	for (auto & argument : arguments)
	{
		if (argument->IsExplicitOrHasExplicitChild())
		{
			return true;
		}
	}
	return false;
}

// todo: should this be passed in as a unique_ptr?
ErrorOr<Success> ParamRepeatableRequired::SetArgumentInternal(CommandContext & context, value_ptr<CommandElement>&& argument)
{
	this->arguments.emplace_back(argument);
	// need to update all members because vector location changes
	for (int i = 0; i < this->arguments.size(); i++)
	{
		this->arguments[i]->location_in_parent = &(this->arguments[i]);
	}
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

bool ParamRepeatableRequired::RemoveLastArgument()
{
	if (arguments.size() > 0)
	{
		arguments.pop_back();
		return true;
	}
	return false;
}

ErrorOr<Value> ParamRepeatableRequired::Evaluate(CommandContext & context) const
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

ErrorOr<std::vector<Value> > ParamRepeatableRequired::EvaluateRepeatable(CommandContext & context) const
{
	std::vector<Value> values;

	for (auto& argument : arguments)
	{
		values.push_back(CHECK_RETURN(argument->Evaluate(context)));
	}
	return values;
}

std::string ParamRepeatableOptional::GetPrintString(std::string line_prefix) const
{
	std::string print_string = "";
	for (auto& arg : arguments)
	{
		print_string += arg->GetPrintString(line_prefix);
	}
	if (arguments.empty() && !default_value.value.empty())
	{
		print_string += line_prefix + "(" + default_value.value + ")\n";
	}

	return print_string;
}

bool ParamRepeatableOptional::IsSatisfied() const
{
	return arguments.empty()
		|| arguments[arguments.size()-1]->ParametersSatisfied();
}


ErrorOr<Value> ParamRepeatableOptional::Evaluate(CommandContext & context) const
{
	if (arguments.size() > 1)
	{
		return Error("There are too many arguments here for evaluating as a single");
	}
	if (arguments.size() == 0)
	{
		// @Incomplete: get default value
		if (default_value.value.empty())
		{
			return Error("Shouldn't call evaluate on an optional parameter without a default value");
		}
		else
		{
			auto default_element = CHECK_RETURN(context.GetNewCommandElement(default_value.value));
			return default_element->Evaluate(context);
		}
	}
	return arguments[0]->Evaluate(context);
}

ErrorOr<std::vector<Value> > ParamRepeatableOptional::EvaluateRepeatable(CommandContext & context) const
{
	std::vector<Value> values;

	if (arguments.size() == 0 && !default_value.value.empty())
	{
		auto default_element = CHECK_RETURN(
			context.GetNewCommandElement(default_value.value));
		values.push_back(CHECK_RETURN(default_element->Evaluate(context)));
	}

	for (auto argument : arguments)
	{
		values.push_back(CHECK_RETURN(argument->Evaluate(context)));
	}
	
	return values;
}

std::string OneOf::GetPrintString(std::string line_prefix) const
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

std::vector<ElementType::Enum> OneOf::GetAllowedTypes() const
{
	if (chosen_index != -1)
	{
		return possibilities[chosen_index]->GetAllowedTypes();
	}
	Set<ElementType::Enum> allowed_set;
	std::vector<ElementType::Enum> allowed;
	for (auto& parameter : possibilities)
	{
		auto param_allowed = parameter->GetAllowedTypes();
		for(auto& type : param_allowed)
		{
			if (allowed_set.count(type) == 0)
			{
				allowed_set.insert(type);
				allowed.push_back(type);
			}
		}
	}
	return allowed;
}

bool OneOf::IsRequired() const
{
	for (auto& parameter : possibilities)
	{
		if (! parameter->IsRequired())
		{
			return false;
		}
	}
	return true;
}

bool OneOf::IsSatisfied() const
{
	if (chosen_index != -1)
	{
		return possibilities[chosen_index]->IsSatisfied();
	}
	for (auto& parameter : possibilities)
	{
		if (parameter->IsSatisfied())
		{
			return true;
		}
	}
	return false;
}

bool OneOf::HasExplicitArgOrChild() const
{
	if (chosen_index != -1)
	{
		return possibilities[chosen_index]->HasExplicitArgOrChild();
	}
	for (auto & parameter : possibilities)
	{
		if (parameter->HasExplicitArgOrChild())
		{
			return true;
		}
	}
	return false;
}


ErrorOr<Success> OneOf::SetArgumentInternal(CommandContext & context, value_ptr<CommandElement>&& argument)
{
	if (chosen_index != -1)
	{
		return possibilities[chosen_index]->SetArgument(context, std::move(argument));
	}
	for (int index = 0; index < possibilities.size(); index++)
	{
		auto & parameter = possibilities[index];
		auto result = parameter->SetArgument(context, std::move(argument));
		if (!result.IsError())
		{
			chosen_index = index;
			break;
		}
		if (argument == nullptr)
		{
			return Error("OneOf SetArgument moved the argument but didn't succeed");
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

bool OneOf::RemoveLastArgument()
{
	if (chosen_index == -1)
	{
		return false;
	}
	bool removed = possibilities[chosen_index]->RemoveLastArgument();
	// if have no more arguments, we no longer have a chosen_index
	if (possibilities[chosen_index]->GetLastArgument() == nullptr)
	{
		chosen_index = -1;
	}
	return removed;
}

ErrorOr<Value> OneOf::Evaluate(CommandContext & context) const
{
	if (chosen_index == -1)
	{
		for (auto& parameter : possibilities)
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

ErrorOr<std::vector<Value> > OneOf::EvaluateRepeatable(CommandContext & context) const
{
	if (chosen_index == -1)
	{
		for (auto& parameter : possibilities)
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
