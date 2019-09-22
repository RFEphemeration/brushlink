#include "CommandTypes.hpp"

using namespace Farb;

namespace Command
{

ErrorOr<Value> ElementParameter::GetDefaultValue(CommandContext context)
{
	if (default_value)
	{
		return *default_value;
	}
	else if (default_element)
	{
		std::map<ParameterIndex, Value>& arguments;
		return default_element->Evaluate(context, arguments);
	}
	return Error("No Default Value Available");
}

bool ElementParameter::Optional()
{
	return (default_value != nullptr || default_element != nullptr);
}

ErrorOr<Success> Element::FillDefaultArguments(
	CommandContext context,
	std::map<ParameterIndex, Value>& arguments) const
{
	if (arguments.count(kLeftParameterIndex) == 0
		&& left_parameter != nullptr)
	{
		if (left_parameter.Optional())
		{
			return Error("Missing required left argument for Element");
		}
		arguments.insert(kLeftParameterIndex,
			CHECK_RETURN(left_parameter.GetDefaultValue(context)));
	}

	for (int arg = 0; arg < right_parameters.count(); arg++)
	{
		if (arguments.count(ParameterIndex(arg)) > 0)
		{
			continue;
		}
		if (!right_parameters[arg].Optional())
		{
			return Error("Missing required argument " + arg + " for Element");
		}
		arguments.insert(ParameterIndex(arg),
			CHECK_RETURN(right_parameters[arg].GetDefaultValue(context)));
	}

	int desired_argument_count = right_parameters.count();
	desired_argument_count += left_parameter != nullptr ? 1 : 0;

	if (arguments.count() != desired_argument_count)
	{
		return Error("Too many arguments for Element");
	}

	return Success();
}

FragmentMapping::FragmentMapping(const std::vector<CRef<Element> > & elements)
{
	for(const Element & e : elements)
	{
		Append(e, false);
	}

	EvaluateOrder();
}

void FragmentMapping::Append(const Element & e, bool evaluateOrder = true)
{
	ElementIndex eIndex { elementMapping.count() };
	elementMapping.push_back(ElementMapping{e});

	if (e.left_parameter != nullptr)
	{
		int leftIndex = FindAppropriateLeftParameter(e);
		Pair<int, int> parentIndex = parents[leftIndex];
		
		parameters[parentIndex.first][parentIndex.second] = newIndex;
		parents[newIndex] = parentIndex;

		parameters[newIndex][kLeftParameterIndex] = leftIndex;
		parents[leftIndex] = Pair(newIndex, kLeftParameterIndex);
	}
	// we are probably a parameter of another element
	else
	{
		// 
	}

	if (!evaluateOrder)
	{
		return;
	}
}


// this is recursive descent
// requires precondition that everything in evaluationOrder so far
// isn't dependent on the element at the provided index
void FragmentMapping::EvaluateOrderFrom(ElementIndex index)
{
	for(const auto & argument : elementMapping[index].arguments)
	{
		EvaluateOrderFrom(argument.second);
	}
	evaluationOrder.push_back(index);
}

ErrorOr<Success> FragmentMapping::EvaluateOrder()
{
	evaluationOrder.clear();

	for (int i = 0; i < elementMapping.count(); i++)
	{
		if (elementMapping[i].parent.first == kNullElementIndex)
		{
			EvaluateOrderFrom(ElementIndex{i});
			break;
		}
	}

	if (evaluationOrder.count() != elementMapping.count())
	{
		return Error("FragmentMapping::EvaluteOrder did not map correctly");
	}

	return Success();
}

// we can't do recursive descent here because ElementReferences
// must be resolved in this scope, where they can access this Word's arguments
// even if they are arguments to other Elements in the implementation
ErrorOr<Value> ElementWord::Evaluate(
	CommandContext context,
	std::map<ParameterIndex, Value> arguments) const
{
	CHECK_RETURN(FillDefaultArguments(context, arguments));

	std::vector<Value> computedValues{implementation.elementMapping.size()};
	std::map<ParameterIndex, Value> subArguments;

	for(ElementIndex index : implementation.evaluationOrder)
	{
		const auto & elementMapping = implementation.elementMapping[index.value];
		const auto & element = elementMapping.element;
		const auto pReference = dynamic_cast<const ElementReference*>(&element);
		
		if (pReference != nullptr)
		{
			// this should be in range because it's been checked in PostLoad
			computedValues[index.value] = arguments[pReference->parameter];
			continue;
		}
		subArguments.clear();
		for(auto [parameterIndex, elementValueIndex] : elementMapping.arguments)
		{
			subArguments[parameterIndex] = computedValues[elementValueIndex.value];
		}
		computedValues[index.value] = element.Evaluate(context, subArguments);
	}

	// last computed value is return, yes? or is this a faulty assumption?
	return computedValues[computedValues.count - 1];
}

ErrorOr<Success> ElementWord::PostLoad()
{
	for(ElementIndex index : implementation.evaluationOrder)
	{
		const auto & element  = implementation.elementMapping[index.value].element;
		const auto pReference = dynamic_cast<const ElementReference*>(&element);
		
		if (pReference == nullptr)
		{
			continue;
		}

		// this reference must be in range of the parameters for this element
		if (pReference->parameter == kLeftParameterIndex
			&& left_parameter == nullptr)
		{
			return Error("ElementWord contains a reference to a left parameter when one doesn't exist");
		}
		else if (pReference->parameter != kLeftParameterIndex
			&& !right_parameters.size() > pReference->parameter.value)
		{
			return Error("ElementWord contains a reference to a right parameter that doesn't exist");
		}
	}

	return Success();
}

} // namespace Command
