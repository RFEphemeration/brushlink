#include "CommandElement.hpp"

namespace Command
{

Table<ElementType::Enum, int> CommandElement::GetAllowedArgumentTypes()
{
	Table<ElementType::Enum, int> allowed;
	int last_param_with_args = -1;

	// find the last parameter that has arguments.
	// we can't go backwards (for now, until we implement permutable)
	// perhaps a way to implement permutable parameters would be to
	// move the first parameter in a permutable sequence to the front
	// that sounds not too hard to do and maintains the structure here
	// @Feature: permutable parameters
	// we could even use this to allow users to set the parameter order
	// perhaps trivially as a new word w/ the same name and swapped params
	for (int index = parameters.size() - 1; index >= 0 ; index--)
	{
		// earlier arguments to parameters can't be revisited
		CommandElement * argument = parameters[index]->GetLastArgument();
		if (argument != nullptr
			&& argument->IsExplicitOrHasExplicitChild())
		{
			last_param_with_args = index;
			for (auto pair : argument->GetAllowedArgumentTypes())
			{
				allowed[pair.first] += pair.second;
			}
			if (!argument->ParametersSatisfied())
			{
				return allowed;
			}
			break;
		}
	}

	for (int index = last_param_with_args + 1; index < parameters.size(); index++)
	{
		CommandElement * argument = parameters[index]->GetLastArgument();
		if (argument != nullptr)
		{
			// this is an implicit argument, recurse on its parameters
			last_param_with_args = index;
			for (auto pair : argument->GetAllowedArgumentTypes())
			{
				allowed[pair.first] += pair.second;
			}
			if (!argument->ParametersSatisfied())
			{
				return allowed;
			}
			continue;
		}

		// there are no arguments in any of these parameters
		// so we just ask the parameter directly
		Set<ElementType::Enum> param_allowed = parameters[index]->GetAllowedTypes();

		// @Cleanup @Performance we could probably do this once in the Parameter
		// rather than every time in the element
		// checking for implied args to param here so that
		// the count (used for skip) matches the number of params
		Set<ElementType::Enum> param_allowed_with_implied;
		for (auto&& pair : CommandContext::allowed_with_implied)
		{
			if (param_allowed.count(pair.first) <= 0)
			{
				continue;
			}
			param_allowed_with_implied.merge(Set<ElementType::Enum>{pair.second});
		}
		param_allowed.merge(param_allowed_with_implied);

		for (auto&& type : param_allowed)
		{
			// @Bug make sure this doesn't initialize to anything other than 0
			// when allowed[type] doesn't already exist
			allowed[type] += 1;
		}

		// @Incomplete permutable
		if (parameters[index]->IsRequired())
		{
			break;
		}
	}

	return allowed;
}

ErrorOr<bool> CommandElement::AppendArgument(CommandContext & context, value_ptr<CommandElement>&& next, int & skip_count)
{
	int last_param_with_args = -1;
	for (int index = parameters.size() - 1; index >= 0 ; index--)
	{
		// earlier arguments to parameters can't be revisited
		CommandElement * argument = parameters[index]->GetLastArgument();
		if (argument != nullptr
			&& argument->IsExplicitOrHasExplicitChild())
		{
			last_param_with_args = index;
			bool result = CHECK_RETURN(argument->AppendArgument(context, std::move(next), skip_count));
			if (result)
			{
				return true;
			}
			break;
		}
	}

	for (int index = last_param_with_args+1; index < parameters.size(); index++)
	{
		CommandElement * argument = parameters[index]->GetLastArgument();
		if (argument != nullptr)
		{
			// this is an implicit argument, recurse on its parameters
			bool result = CHECK_RETURN(argument->AppendArgument(context, std::move(next), skip_count));
			if (result)
			{
				return true;
			}
			continue;
		}
		
		auto allowed_types = parameters[index]->GetAllowedTypes();
		// without implied types for same type take priority
		// and we only want to decrement skip count at most once per parameter
		if (allowed_types.count(next->Type()) > 0)
		{
			if (skip_count == 0)
			{
				CHECK_RETURN(parameters[index]->SetArgument(context, std::move(next)));
				return true;
			}
			skip_count--;
		}
		else
		{
			for(auto&& arg_type : allowed_types)
			{
				if (CommandContext::allowed_with_implied.count(arg_type) <= 0
					|| CommandContext::allowed_with_implied.at(arg_type).count(next->Type()) <= 0)
				{
					continue;
				}
				if (skip_count > 0)
				{
					// if we match in one way or 10 ways, we still skip once
					skip_count--;
					break;
				}
				
				ElementName implied_name = CommandContext::implied_elements.at(next->Type()).at(arg_type);
				auto implied_element = CHECK_RETURN(context.GetNewCommandElement(implied_name.value));
				implied_element->implicit = Implicit::Parent;
				int no_skips = 0;
				bool result = CHECK_RETURN(implied_element->AppendArgument(context, std::move(next), no_skips));
				if (!result)
				{
					return Error("Could not append new element to implied element");
				}
				CHECK_RETURN(parameters[index]->SetArgument(context, std::move(implied_element)));
				return true;
			}
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

bool CommandElement::IsExplicitOrHasExplicitChild()
{
	if (implicit == Implicit::None)
	{
		return true;
	}
	for (auto& parameter : parameters)
	{
		if (parameter->HasExplicitArgOrChild())
		{
			return true;
		}
	}
	return false;
}

ErrorOr<bool> CommandElement::RemoveLastExplicitElement()
{
	bool had_explicit_child = false;
	for (int index = parameters.size() - 1; index >= 0 ; index--)
	{
		// earlier arguments to parameters can't be revisited
		CommandElement * argument = parameters[index]->GetLastArgument();
		// can the last argument of a parameter be implicit but
		// earlier ones explicit?
		// if so then we would need GetLastExplicitArgument
		if (argument != nullptr
			&& argument->IsExplicitOrHasExplicitChild())
		{
			had_explicit_child = true;
			bool should_remove_argument = CHECK_RETURN(argument->RemoveLastExplicitElement());
			if (should_remove_argument)
			{
				bool removed = parameters[index]->RemoveLastArgument();
				if (!removed)
				{
					return Error("Expected to RemoveLastArgument but couldn't");
				}
				break;
			}
			else
			{
				break;
			}
		}
	}
	if (!had_explicit_child)
	{
		// we had no explicit arguments to remove
		// we are a leaf, our parent should remove us
		return true;
	}
	else if (implicit == Implicit::Parent
		&& !IsExplicitOrHasExplicitChild())
	{
		// we have no explicit children anymore and are an implict parent
		// we should also be removed by our parent
		return true;
	}
	else
	{
		return false;
	}
}

bool CommandElement::ParametersSatisfied()
{
	for (auto& parameter : parameters)
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

	for (auto& filter : filters)
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
