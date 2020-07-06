#include "Parameter.hpp"
#include "Element.hpp"

namespace Command
{

template<bool repeatable, bool optional>
Element * Parameter_Basic<repeatable, optional>::GetLastArgument()
{
	if (arguments.empty())
	{
		return nullptr;
	}
	return arguments.back().get();
}

template<bool repeatable, bool optional>
std::string Parameter_Basic<repeatable, optional>::GetPrintString(std::string line_prefix) const
{
	if constexpr(optional)
	{
		if (arguments.empty() && default_value.has_value)
		{
			return line_prefix + "(" + default_value.value() + ")\n";
		}
	}
	std::string print_string = "";
	for (auto arg : arguments)
	{
		print_string += arg->GetPrintString(line_prefix);
		if constexpr(!repeatable)
		{
			break;
		}
	}
	return print_string;
}

template<bool repeatable, bool optional>
bool Parameter_Basic<repeatable, optional>::IsSatisfied() const
{
	for (auto & arg : arguments)
	{
		if (!arg->IsSatisfied())
		{
			return false;
		}
	}
	if constexpr(optional)
	{
		return true;
	}
	else
	{
		return !arguments.empty();
	}
}

template<bool repeatable, bool optional>
void Parameter_Basic<repeatable, optional>::GetAllowedTypes(AllowedTypes & allowed) const
{
	if (!arguments.empty())
	{
		arguments.back()->GetAllowedTypes(allowed);
	}
	if (repeatable || arguments.empty())
	{
		auto types = Types();
		for (auto && type : types)
		{
			allowed.Append({type});
		}
		auto allowed_types = context.GetAllowedWithImplied({types});
		for (auto && type : allowed_types)
		{
			allowed.append({type});
		}
	}
}

template<bool repeatable, bool optional>
bool Parameter_Basic<repeatable, optional>::IsExplicitBranch() const
{
	for(auto arg & arguments)
	{
		if (arg->IsExplicitBranch())
		{
			return true;
		}
	}
	return false;
}

template<bool repeatable, bool optional>
ErrorOr<bool> Parameter_Basic<repeatable, optional>::AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count)
{
	if (!arguments.empty())
	{
		auto appended = CHECK_RETURN(arguments.back()->AppendArgument(context, next, skip_count));
		if (appended)
		{
			return true;
		}
		if constexpr (!repeatable)
		{
			return false;
		}
	}

	if (!optional && arguments.empty())
	{
		if (skip_count > 0)
		{
			return Error("Skipping required argument");
		}
		if (type != next->type)
		{
			auto allowed_implied_set = context.GetAllowedWithImplied({type});
			if (!Contains(allowed_implied_set, next->type))
			{
				return Error("Type mismatch on required argument");
			}
		}
	}
	// should we be type checking here?

	if (skip_count > 0)
	{
		skip_count--;
		return false;
	}

	auto args_capacity = arguments.capacity();
	if (type == next->type)
	{
		arguments.push_back(std::move(next));
		arguments.back()->location_in_parent = &arguments.back();
	}
	else
	{
		ElementName implied_name = context.GetImpliedElement(next->type, type);
		auto implied_element = CHECK_RETURN(context.GetNewCommandElement(implied_name));
		implied_element->implicit = Implicit::Parent;
		int no_skips = 0;
		bool result = CHECK_RETURN(implied_element->AppendArgument(context, std::move(next), no_skips));
		if (!result)
		{
			return Error("Could not append new element to implied element");
		}
		arguments.push_back(std::move(implied_element));
		arguments.back()->location_in_parent = &arguments.back();
	}

	if (arguments.capacity() != args_capacity)
	{
		// we have moved the location of the other arguments by reallocating
		for (auto arg & : arguments)
		{
			arg->location_in_parent = &arg;
		}
	}
	return true;
}

template<bool repeatable, bool optional>
ErrorOr<Removal> Parameter_Basic<repeatable, optional>::RemoveLastExplicitElement()
{
	if (arguments.empty())
	{
		return Removal::None;
	}
	auto & arg = arguments.back();
	Removal removal = CHECK_RETURN(arg->RemoveLastExplicitElement());
	if (removal == Removal::Finished)
	{
		return Removal::Finished;
	}

	// if there are still explicit right children, we are finished or in error
	for (auto & param : arg->parameters)
	{
		if (param->IsExplicitBranch())
		{
			if (removal == Removal::None)
			{
				return Error("Couldn't remove explicit argument from element");
			}
			return Removal::Finished;
		}
	}

	if (removal == Removal::ContinueRemovingImplicit)
	{
		if (arg->implicit == Implicit::None)
		{
			return Removal::Finished;
		}
		else if (arg->implicit == Implicit::Child)
		{
			if (arg->left_parameter
				&& arg->left_parameter->GetLastArgument())
			{
				return Error("An implicit child element has a left parameter");
			}
			// an implied child is paired with an explicit parent
			// if we've already removed an explicit descendent, we're done
			return Removal::Finished;
		}
		// if implicit parent, continue to removal code below
	}

	// removing this element, first by trying to swap with left
	if (arg->left_parameter
		&& arg->left_parameter->GetLastArgument())
	{
		value_ptr<Element> boxed;
		arg.swap(boxed);
		Element * left_element = arg->left_parameter->GetLastArgument();
		arg.swap(*left_element->location_in_parent);
	}
	// if no left swap, just remove it entirely
	else
	{
		if (arg->implicit == Implicit::Child)
		{
			for(auto & argument : arguments)
			{
				if (argument->implicit != Implicit::Child)
				{
					return Error("Implicit child argument comes after explicit");
				}
			}
			return Removal::None;
		}
		
		arguments.pop_back();
		// because we just removed an element, parents should re-check their implicity
		return Removal::ContinueRemovingImplicit;
	}
	
}

template<bool repeatable, bool optional>
ErrorOr<Variant> Parameter_Basic<repeatable, optional>::Evaluate(Context & context) const override
{
	auto count = arguments.size();
	if (count == 1)
	{
		return arguments.front()->Evaluate(context);
	}
	else if (count == 0)
	{
		if constexpr (optional)
		{
			if (default_value.has_value())
			{
				value_ptr<Element> default_element = CHECK_RETURN(context.GetElement(default_value.value()));
				return default_element->Evaluate(context);
			}
		}
		return Error("Parameter has no argument or default");
	}
	else
	{
		return Error("Parameter has multiple arguments, should only have one");
	}
}

template<bool repeatable, bool optional>
ErrorOr<std::vector<Variant> > Parameter_Basic<repeatable, optional>::EvaluateRepeatable(Context & context) const override
{
	if constexpr (optional)
	{
		if (arguments.empty() && default_value.has_value())
		{
			value_ptr<Element> default_element = CHECK_RETURN(context.GetElement(default_value.value()));
			return std::vector<Variant>{
				CHECK_RETURN(default_element->Evaluate(context))
			};
		}
	}
	std::vector<Variant> values;
	for(auto & arg : arguments)
	{
		values.push_back(CHECK_RETURN(arg->Evaluate(context)));
	}
	return values;
}

template struct Parameter_Basic<false, false>;
template struct Parameter_Basic<true, false>;
template struct Parameter_Basic<false, true>;
template struct Parameter_Basic<true, true>;

Set<Variant_Type> Parameter_OneOf::Types() const
{
	if (chosen_index.has_value())
	{
		return options[chosen_index.value()]->Types();
	}
}

std::string Parameter_OneOf::GetPrintString(std::string line_prefix) const
{

}

bool Parameter_OneOf::IsSatisfied() const
{

}

void Parameter_OneOf::GetAllowedTypes(AllowedTypes & allowed) const
{

}

ErrorOr<bool> Parameter_OneOf::AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count)
{

}

ErrorOr<Variant> Parameter_OneOf::Evaluate(Context & context) const
{

}

ErrorOr<std::vector<Variant> > Parameter_OneOf::EvaluateRepeatable(Context & context) const
{

}


} // namespace Command
