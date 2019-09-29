#include "CommandTypes.hpp"

using namespace Farb;

namespace Command
{

bool ElementDeclaration::HasLeftParamterMatching(ElementType type) const
{
	if (left_parameter == nullptr) return false;
	return type & left_parameter->types;
}

ErrorOr<ElementParameter> ElementDeclaration::GetParameter(ParameterIndex index) const
{
	if (index == kLeftParameterIndex
		&& left_parameter != nullptr)
	{
		return *left_parameter; 
	}
	else if (index > kLeftParameterIndex && < right_parameters.size())
	{
		return right_parameters[index];
	}
	return Error("Index out of range");
}

ParameterIndex ElementDeclaration::GetMaxParameterIndex() const
{
	if (right_parameters.size() == 0 && left_parameter == nullptr)
	{
		return kNullParameterIndex;
	}
	else if (right_parameters.size() > 0)
	{
		return ParameterIndex { right_parameters.size() - 1 };
	}
	else if (left_parameter != nullptr)
	{
		return kLeftParameterIndex;
	}
}

inline ParameterIndex ElementDeclaration::GetMinParameterIndex() const
{
	if (left_parameter != nullptr)
	{
		return kLeftParameterIndex;
	}
	return ParameterIndex { 0 };
}

// rmf todo: change this to a multimap
ErrorOr<Success> ElementDeclaration::FillDefaultArguments(
	std::map<ParameterIndex, ElementToken>& arguments) const
{
	if (arguments.count(kLeftParameterIndex) == 0
		&& left_parameter != nullptr)
	{
		if (!left_parameter.optional)
		{
			return Error("Missing required left argument for Element");
		}
		arguments.insert(kLeftParameterIndex, left_parameter.default_value);
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
	desired_argument_count += left_parameter != nullptr ? 1 : 0;

	if (arguments.count() != desired_argument_count)
	{
		return Error("Too many arguments for Element");
	}

	return Success();
}

ErrorOr<ElementNode &> ElementNode::Add(ElementNode child, ParameterIndex argIndex)
{
	// this is often passed in after already walking the args and params
	if (argIndex == kNullParameterIndex)
	{
		argIndex = CHECK_RETURN(GetArgIndexForNextToken(child.token));
	}
	
	auto childIndex = ElementIndex{children.size()};
	child.parent = this;
	children.push_back(child);
	childArgumentMapping.insert( { argIndex, childIndex } );
	children[childIndex.value].UpdateChildrenSetParent();
	return children[childIndex.value];
}

void ElementNode::UpdateChildrenSetParent()
{
	for (auto child : children)
	{
		child.parent = this;
		child.UpdateChildrenSetParent();
	}
}

bool ElementNode::ParametersMet() const;
{
	return WalkArgsAndParams().allParametersMet;
}

ElementType ElementNode::GetValidNextArgTypes() const
{
	return WalkArgsAndParams().validNextArgs;
}

ElementType ElementNode::GetValidNextArgsWithImpliedNodes() const
{
	return WalkArgsAndParams().validNextArgsWithImpliedNodes;
}

ErrorOr<ParameterIndex> GetArgIndexForNextToken(ElementToken token) const
{
	auto result = WalkArgsAndParams(token.types);
	ParameterIndex index = result.firstArgIndexForNextToken;
	if (index == kNullParameterIndex)
	{
		return Error("There was no valid place for this token");
	}
	else if (result.firstArgRequiresImpliedNode)
	{
		return Error("This token requires an implied token");
	}
	return index;
}

void ElementNode::ArgAndParamWalkResult::AddValidNextArg(ParameterIndex index, ElementType types)
{
	validNextArgs = validNextArgs | types;

	ElementType typesWithImplied { 0 };

	for (auto pair : ImpliedNodeOptions::potentialParamTypes)
	{
		if (pair.first & types)
		{
			typesWithImplied = typesWithImplied | pair.second;
		}
	}

	validNextArgsWithImpliedNodes = validNextArgsWithImpliedNodes |  typesWithImplied;

	if ((firstArgIndexForNextToken == kNullParameterIndex
			|| index < firstArgIndexForNextToken)
		&& (type & nextTokenType
			|| typesWithImplied & nextTokenType))
	{
		if (type & nextTokenType)
		{
			firstArgRequiresImpliedNode = false;
		}
		else
		{
			firstArgRequiresImpliedNode = true;
		}
		firstArgIndexForNextToken = index;
	}
}

ElementNode::ArgAndParamWalkResult ElementNode::WalkArgsAndParams(ElementType nextTokenType = ElementType { 0 }) const
{
	ArgAndParamWalkResult result;
	result.nextTokenType = nextTokenType;

	const ElementDeclaration & dec = GetElementDeclaration(nextToken);
	ParameterIndex maxArgIndex = kNullParameterIndex;
	auto lastMapping = childArgumentMapping.rbegin();
	if (lastMapping != childArgumentMapping.rend())
	{
		maxArgIndex = lastMapping->first;
	}
	ParameterIndex minParamIndex = dec.GetMinParameterIndex();
	ParameterIndex maxParamIndex = dec.GetMaxParameterIndex();

	for (ParameterIndex index { maxArgIndex.value };
		index.value > minParamIndex.value;
		index.value--)
	{
		ElementParameter param = CHECK_RETURN(dec.GetParameter(index));
		if (!param.permutable && index != maxArgIndex)
		{
			// don't have to worry about any parameters before the most recent one if they aren't permutable
			break;
		}
		if (!childArgumentMapping.contains(index)
			|| param.repeatable)
		{
			result.AddValidNextArg(index, param.types);
		}
		if (!childArgumentMapping.contains(index)
			&& !param.optional)
		{
			result.allParametersMet = false;
		}
		if (!param.permutable && index == maxArgIndex)
		{
			// if the maxArgIndex isn't permutable, none of the ones
			// before it matter at all
			break;
		}
	}

	// rmf todo: this logic is a mess
	bool inPermutableSection = false;
	bool allPermutableWereOptional = true;
	for (ParameterIndex index { maxArgIndex.value + 1};
		index.value <= maxParamIndex.value;
		index.value++)
	{
		ElementParameter param = CHECK_RETURN(dec.GetParameter(index));

		if (inPermutableSection
			&& !param.permutable)
		{
			if (!allPermutableWereOptional)
			{
				// this is the end of a permutable section
				// with required parameters, so we can't go past here
				result.allParametersMet = false;
				break;
			}
			// if we're going to continue, reset permutable
			allPermutableWereOptional = true;
			inPermutableSection = false;
		}
		if (param.permutable)
		{
			inPermutableSection = true;
		}

		result.AddValidNextArg(index, param.types);

		if (!param.optional)
		{
			result.allParametersMet = false;
			if (inPermutableSection)
			{
				allPermutableWereOptional = false;
			}
		}

		if (!param.optional
			&& !inPermutableSection)
		{
			// this is a required parameter that is not permutable
			// so we can't go past here
			break;
		}
	}

	return result;
}

bool ElementMapping::ParametersMet() const
{
	if (element.left_parameter != nullptr
		&& !element.left_parameter.Optional()
		&& !arguments.contains_key(kLeftParameterIndex))
	{
		return false;
	}

	for (ParameterIndex p{0}; p.value < element.right_parameters.size(); p.value++)
	{
		if (!element.right_parameters[p.value].Optional()
			&& !arguments.contains_key(p))
		{
			return false;
		}
	}

	return true;
}

FragmentMapping::FragmentMapping(const std::vector<CRef<Element> > & elements)
{
	for(const Element & e : elements)
	{
		Append(e, false);
	}

	EvaluateOrder();
}

ErrorOr<Success>() FragmentMapping::Append(const Element & e, bool evaluateOrder = true)
{
	ElementIndex eIndex { elementMapping.count() };
	elementMapping.push_back(ElementMapping{e});

	if (e.left_parameter != nullptr)
	{
		auto leftIndex = FindAppropriateLeftParameter(e);
		if (!e.left_parameter.Optional()
			&& leftIndex == kNullElementIndex)
		{
			return Error("Left Parameter is required but could not be found");
		}
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

ElementIndex FragmentMapping::FindAppropriateLeftParameter(const Element & next) const
{
	if (next.left_parameter == nullptr
		|| elementMapping.empty())
	{
		return kNullElementIndex;
	}

	const ValueType desiredType = next.left_parameter->type;
	ElementIndex current{elementMapping.count() - 1};
	while (elementMapping[current.value].element.type == )
	while (current.value > kNullElementIndex)
	{
		if (!elementMapping[current.value].ParametersMet())
		{
			return kNullElementIndex;
		}
		else if (elementMapping[current.value].element.type == desiredType)
		{
			// rmf todo: implement left param skipping based on count of skip elements
			// which ones count? only ones typed last, right?
			// so maybe our current index initialization is wrong
			break;
		}
		else
		{
			current = elementMapping[current.value].parent.first;
		}
	}

	return current;
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
