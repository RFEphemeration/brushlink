#include "Element.hpp"
#include "Parameter.hpp"

namespace Command
{

// bool is whether parameters are satisfied
std::pair<std::vector<Parameter*>, bool> GetActiveParameters(Element & element)
{
	// left parameters are never considered active
	// and if this element exists its left parameter is assumed to be satisfied

	// @Feature permutable
	std::vector<Parameter*> active;
	int first_active_param = 0;
	for (int index = element.parameters.size() - 1; index >= 0 ; index--)
	{
		// arguments to parameters before the most recent explicit can't be revisited
		Element * argument = element.parameters[index]->GetLastArgument();
		if (argument != nullptr
			&& argument->IsExplicitBranch())
		{
			first_active_param = index;
			break;
		}
	}

	for(int index = first_active_param; index < element.parameters.size(); index++)
	{
		active.push_back(element.parameters[index].get());
		if (!element.parameters[index]->IsSatisfied())
		{
			return {active, false};
		}
	}

	return {active, true};
}

bool Element::IsSatisfied() const
{
	// aren't left parameters assumed to be satisfied?
	// if they aren't, should they be part of active parameters? no
	if (left_parameter
		&& !left_parameter->IsSatisfied())
	{
		return false;
	}
	for (auto param & : parameters)
	{
		if (!parameter.IsSatisfied())
		{
			return false;
		}
	}
	return true;
}

bool Element::IsExplicitBranch() const override
{
	if (implicit == Implicit::None)
	{
		return true;
	}
	// @Feature LeftParam do we need to check left parameter here?
	if(left_parameter
		&& left_parameter->IsExplicitBranch())
	{
		return true;
	}
	for (auto& parameter : parameters)
	{
		if (parameter->IsExplicitBranch())
		{
			return true;
		}
	}
	return false;
}

void Element::GetAllowedTypes(AllowedTypes & allowed) const
{
	auto [ active, satisfied ] = GetActiveParameters(*this);

	for (auto * param : active)
	{
		param->GetAllowedTypes(allowed);
		if (!param->IsSatisfied())
		{
			return;
		}
	}
	if (satisfied)
	{
		// this is a left argument
		allowed.Append({Type(), true});
	}
	return;
}

ErrorOr<bool> Element::AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count)
{
	auto [ active, satisfied ] = GetActiveParameters(*this);
	for (auto * param : active)
	{
		auto result = CHECK_RETURN(param->AppendArgument(context, next, skip_count));
		if (result)
		{
			return true;
		}
	}

	if (!satisfied)
	{
		return Error("Can't append because a preceding parameter has not been satisfied");
	}

	// left parameters have lower priority than right parameters
	// because after left parameters are applied you can't access any right parameters

	// the requirements for left parameters are kind of ridiculous
	// @Feature LeftParam types don't need to be ==, just acceptable to parent
	// but it should probably be equal unless there's a very good reason not to
	if (next->left_parameter == nullptr
		|| next->left_parameter->GetLastArgument() != nullptr
		|| next->type != type
		|| !Contains(next->left_parameter->GetAllowedTypes(), type)
		|| location_in_parent == nullptr)
	{
		return false;
	}

	if (skip_count > 0)
	{
		skip_count--;
		return false;
	}

	if (this != location_in_parent->get())
	{
		return Error("location_in_parent has a mismatched pointer");
	}
	// caching because SetArgument will change this;
	auto * loc = location_in_parent;
	int no_skips = 0;
	value_ptr<Element> boxed_this { loc->release() };
	// append isn't quite the right function because we don't want to
	// have this become a right child of next's left_parameter, right?
	// or does this open up implied left parameters? is that okay?
	auto result = CHECK_RETURN(next->left_parameter->AppendArgument(
		context,
		std::move(boxed_this),
		no_skips));
	if (!result)
	{
		// is this an error after std::move? implication is that the move failed
		loc->reset(boxed_this.release());
		return Error("Unexpectedly failed to append to left");
	}
	next->location_in_parent = loc;
	loc->reset(next.release());

	return true;
}

ErrorOr<Removal> Element::RemoveLastExplicitElement()
{
	bool had_explicit_child = false;
	for (int index = parameters.size() - 1; index >= 0 ; index--)
	{
		if (parameters[index]->IsExplicitBranch())
		{
			Removal removal = CHECK_RETURN(parameters[index]->RemoveLastExplicitElement());
			if (removal == Removal::None)
			{
				return Error("Couldn't remove an explicit argument");
			}
			return removal;
		}
	}
	return Removal::None;
}

} // namespace Command
