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

ParameterIndex ElementNode::RemoveLastChild()
{	
	if (children.size() == 0)
	{
		return kNullParameterIndex;
	}
	children.pop_back();

	ParameterIndex index = kNullParameterIndex;
	
	ElementIndex removed { children.size() };

	auto it = childArgumentMapping.begin()
	while(it != childArgumentMapping.end())
	{
		if (it->second == removed)
		{
			index = it->first;
			it = childArgumentMapping.erase(it);
			
		}
		else
		{
			++it;
		}
	}

	return index;
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

void ElementNode::ArgAndParamWalkResult::AddValidNextArg(ElementType nextTokenType, ParameterIndex index, ElementType types)
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

// rmf todo: this probably shouldn't handle left parameters, right?
// since we'd be appending, and you can't give a left parameter by appending
ElementNode::ArgAndParamWalkResult ElementNode::WalkArgsAndParams(ElementType nextTokenType = ElementType { 0 }) const
{
	ArgAndParamWalkResult result;

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
			result.AddValidNextArg(nextTokenType, index, param.types);
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

		result.AddValidNextArg(nextTokenType, index, param.types);

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

ElementNode * Parser::GetRightmostElement() const
{
	ElementNode * current = root;
	// we must have at least one non left parameter child
	// and there can be at most one left parameter
	while (
		(current->children.size() == 1
			&& !current->childArgumentMapping.contains(kLeftParameterIndex))
		|| current->children.size() > 1)
	{
		current = &current->children[current->children.size() - 1];
	}
	return current;
}

void Parser::ASTWalkResult::AddNodeWalkResult(
	ElementNode * node,
	ElementNode::ArgAndParamWalkResult nodeWalkResult)
{
	potential.validNextArgs = potential.validNextArgs
		| nodeWalkResult.validNextArgs
		| nodeWalkResult.validNextArgsWithImpliedNodes;

	if (nodeWalkResult.firstArgIndexForNextToken.value > kNullParameterIndex.value
		&& !foundValidLocation)
	{
		foundValidLocation = true;
		tokenLocation.e = node;
		tokenLocation.isLeftParam = false;
		tokenLocation.argIndex = nodeWalkResult.firstArgIndexForNextToken;
		tokenLocation.argRequiresImpliedNode = nodeWalkResult.firstArgRequiresImpliedNode;
	}
}

void Parser::ASTWalkResult::AddTypesForPotentialLeftParams(
	ElementNode * node,
	ElementDeclaration * declaration)
{
	potential.rightSideTypesForLeftParameter =
		potential.rightSideTypesForLeftParameter | node->token.types;

	if (!foundValidLocation
		&& nextDec != nullptr
		&& nextDec->HasLeftParamterMatching(e->token.types))
	{
		foundValidLocation = true;
		tokenLocation.e = node;
		tokenLocation.isLeftParam = true;
	}
}

ASTWalkResult Parser::WalkAST(ElementToken * nextToken, bool breakOnFoundLocation = false) const
{
	ElementType nextTokenType = 
		nextToken == nullptr
		? ElementType { 0 }
		: nextToken.type;
	const ElementDeclaration * nextDec =
		nextToken == nullptr
		? nullptr
		: GetElementDeclaration(nextToken);

	ASTWalkResult astWalkResult;

	for(ElementNode * e = GetRightmostElement();
		e != nullptr;
		e = e->parent)
	{
		auto nodeWalkResult = e->WalkArgsAndParams(nextTokenType);

		astWalkResult.AddNodeWalkResult(e, nodeWalkResult);

		if (breakOnFoundLocation
			&& astWalkResult.foundValidLocation)
		{
			// early out for when we're appending and don't need other potentials
			// in the case where we are a right argument
			break;
		}
		else if (!nodeWalkResult.allParametersMet)
		{
			// until parameters have been met, e can't be a left parameter
			// and e->parent can't accept any more arguments
			// this ends our walk
			break;
		}

		astWalkResult.AddTypesForPotentialLeftParams(e, nextDec);

		if (breakOnFoundLocation
			&& astWalkResult.foundValidLocation)
		{
			// early out for when we're appending and don't need other potentials
			// in the case where we found a left argument
			break;
		}

		// for now we're assuming that left parameters can't have implied nodes
		// otherwise that would happen here
	}

	return astWalkResult;
}

// rmf todo: use ast walk result for this
ErrorOr<Success> Parser::Append(ElementToken newToken)
{
	//ElementNode nextNode {nextToken, ElementIndex{stream.size()}};
	//stream.push_back(nextToken);

	if (nodeWalkResult.firstArgIndexForNextToken.value > kNullParameterIndex.value)
	{
		if (!nodeWalkResult.firstArgRequiresImpliedNode)
		{
			CHECK_RETURN(e->Add(nextNode, nodeWalkResult.firstArgIndexForNextToken));
		}
		else
		{
			ElementNode implied {
				ImpliedNodeOptions::acceptedArgTypes[nextToken.type],
				kNullElementIndex };
			CHECK_RETURN(implied.Add(nextNode));
			CHECK_RETURN(e->Add(implied, nodeWalkResult.firstArgIndexForNextToken));
		}

		return Success();
	}

	if (isLeftParam)
	{
		ElementNode * parent = e->parent;
		CHECK_RETURN(nextNode.Add(e, kLeftParameterIndex));
		parent->children.pop_back();
		CHECK_RETURN(parent->Add(nextNode, nodeWalkResult.firstArgIndexForNextToken));
		return Success();
	}
}


ElementNode * Parser::GetValidParent(ElementToken newToken) const
{
	ElementNode * current = GetRightmostElement();

	ElementType validArguments = current->GetValidNextArguments();
	if (validArguments & newToken.types)
	{
		return current;
	}
	while (current->ParametersMet() && current->parent != nullptr)
	{
		current = current->parent;
		validArguments = current->GetValidNextArguments();
		if (validArguments & newToken.types)
		{
			return current;
		}
	}
	return nullptr;

}

ElementType Parser::GetRightSideTypesForLeftParameter() const
{
	ElementType validArguments {0};

	for(ElementNode * e = GetRightmostElement();
		e != nullptr;
		e = e->parent)
	{
		if (!e->ParametersMet())
		{
			break;
		}
		validArguments = validArguments | e->token.types;
	}

	return validArguments;
}

// remember it's also valid to append anything that has a left parameter
// that is on the right
ElementType Parser::GetValidNextArguments() const
{
	ElementType validArguments {0}

	for(ElementNode * e = GetRightmostElement();
		e != nullptr;
		e = e->parent)
	{
		auto walkResult = e->WalkArgsAndParams();
		validArguments = validArguments
			| walkResult.validNextArgs
			| walkResult.validNextArgsWithImpliedNodes;
		
		if (!walkResult.parametersMet)
		{
			break;
		}
	}

	return validArguments;
}

} // namespace Command
