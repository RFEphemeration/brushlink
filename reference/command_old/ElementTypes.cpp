#include "ElementTypes.hpp"

#include "ReflectionDeclare.h"
#include "ReflectionBasics.h"

#include "ElementDictionary.h"

using namespace Farb;

namespace Command
{

ElementToken::ElementToken(ElementName name)
	: name(name)
	, type(ElementDictionary::GetDeclaration(name)->types)
{ }


bool ElementDeclaration::HasLeftParamterMatching(ElementType::Enum type) const
{
	if (left_parameter.get() == nullptr) return false;
	return type & left_parameter->types;
}

ErrorOr<ElementParameter> ElementDeclaration::GetParameter(ParameterIndex index) const
{
	if (index == kLeftParameterIndex
		&& left_parameter.get() != nullptr)
	{
		return *left_parameter; 
	}
	else if (index.value > kLeftParameterIndex.value
		&& index.value < right_parameters.size())
	{
		return right_parameters[index.value];
	}
	return Error("Index out of range");
}

ParameterIndex ElementDeclaration::GetMaxParameterIndex() const
{
	if (right_parameters.size() == 0 && left_parameter.get() == nullptr)
	{
		return kNullParameterIndex;
	}
	else if (right_parameters.size() > 0)
	{
		return ParameterIndex { right_parameters.size() - 1 };
	}
	else if (left_parameter.get() != nullptr)
	{
		return kLeftParameterIndex;
	}

	return kNullParameterIndex;
}

ParameterIndex ElementDeclaration::GetMinParameterIndex() const
{
	if (left_parameter.get() != nullptr)
	{
		return kLeftParameterIndex;
	}
	return ParameterIndex { 0 };
}

// rmf todo: change this to a multimap
/*
ErrorOr<Success> ElementDeclaration::FillDefaultArguments(
	std::map<ParameterIndex, ElementToken>& arguments) const
{
	if (arguments.count(kLeftParameterIndex) == 0
		&& left_parameter.get() != nullptr)
	{
		if (!left_parameter->optional)
		{
			return Error("Missing required left argument for Element");
		}
		arguments.insert(kLeftParameterIndex, (left_parameter->default_value));
	}

	for (int arg = 0; arg < right_parameters.count(); arg++)
	{
		if (arguments.count(ParameterIndex{arg}) > 0)
		{
			continue;
		}
		if (!right_parameters[arg].optional)
		{
			return Error("Missing required argument " + arg + " for Element");
		}
		arguments.insert(ParameterIndex{arg}, right_parameters[arg].default_value);
	}

	int desired_argument_count = right_parameters.count();
	desired_argument_count += left_parameter.get() != nullptr ? 1 : 0;

	if (arguments.count() != desired_argument_count)
	{
		return Error("Too many arguments for Element");
	}

	return Success();
}
*/


} // namespace Command
