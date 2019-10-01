#include "ElementTypes.hpp"

using namespace Farb;

namespace Command
{


const ElementToken ImpliedNodeOptions::selectorToken { ElementName{"Selector"}, ElementType::Selector };
const ElementToken ImpliedNodeOptions::locationToken { ElementName{"Location"}, ElementType::Location };

// accepted arg type -> token to use for node
const std::unordered_map<ElementType, ElementToken> ImpliedNodeOptions::acceptedArgTypes
{
	{ ElementType::Selector_Base, selectorToken },
	{ ElementType::Selector_Group_Size, selectorToken },
	{ ElementType::Selector_Generic, selectorToken },
	{ ElementType::Selector_Superlative, selectorToken },

	{ ElementType::Point, locationToken },
	{ ElementType::Line, locationToken },
	{ ElementType::Area, locationToken }
};

// parameter type -> arg types
const std::unordered_map<ElementType, ElementType> ImpliedNodeOptions::potentialParamTypes
{
	{
		ElementType::Selector,

		ElementType::Selector_Base
		| ElementType::Selector_Group_Size
		| ElementType::Selector_Generic
		| ElementType::Selector_Superlative
	},
	{
		ElementType::Location,

		ElementType::Point
		| ElementType::Line
		| ElementType::Area
	}
};

const std::map<ElementName, ElementDeclaration> ElementDictionary::declarations
{
	{ ElementName{"Attack"},
		{ ElementName{"Attack"}, ElementType::Action,
			std::vector<ElementParameter>
			{
				{ ElementType::Selector }
			}
		}
	},
	{ ElementName{"Enemies"},
		{ ElementName{"Enemies"}, ElementType::Selector_Base }
	},
	{ ElementName{"Within_Range"},
		{ ElementName{"Within_Range"}, ElementType::Selector_Generic,
			std::vector<ElementParameter>
			{
				{ ElementType::Number, true, { ElementName{"Zero"}, ElementType::Number} }
			}
		}
	},
	{ ElementName{"Zero"},
		{ ElementName{"Zero"}, ElementType::Number }
	},
	{ ElementName{"One"},
		{ ElementName{"One"}, ElementType::Number}
	},
	{ ElementName{"Selector"},
		{ ElementName{"Selector"}, ElementType::Selector,
			std::vector<ElementParameter>
			{
				{ ElementType::Selector_Base, true },
				{ ElementType::Selector_Generic, true },
				{ ElementType::Selector_Group_Size, true },
				{ ElementType::Selector_Superlative, true },
			}
		}
	}
};


bool ElementDeclaration::HasLeftParamterMatching(ElementType type) const
{
	if (left_parameter.get() == nullptr) return false;
	return (type & left_parameter->types) != ElementType{0};
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

inline ParameterIndex ElementDeclaration::GetMinParameterIndex() const
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

ErrorOr<ElementNode *> ElementNode::Add(ElementNode child, ParameterIndex argIndex)
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
	children.back().UpdateChildrenSetParent();
	return &children.back();
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

	auto it = childArgumentMapping.begin();
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

bool ElementNode::ParametersMet() const
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

ErrorOr<ParameterIndex> ElementNode::GetArgIndexForNextToken(ElementToken token) const
{
	auto result = WalkArgsAndParams(token.type);
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
		if ((pair.first & types) != ElementType {0})
		{
			typesWithImplied = typesWithImplied | pair.second;
		}
	}

	validNextArgsWithImpliedNodes = validNextArgsWithImpliedNodes |  typesWithImplied;

	if ((firstArgIndexForNextToken == kNullParameterIndex
			|| index.value < firstArgIndexForNextToken.value)
		&& ((types & nextTokenType) != ElementType{0}
			|| (typesWithImplied & nextTokenType) != ElementType{0}))
	{
		if ((types & nextTokenType) != ElementType{0})
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
ElementNode::ArgAndParamWalkResult ElementNode::WalkArgsAndParams(ElementType nextTokenType /* = ElementType { 0 } */) const
{
	ArgAndParamWalkResult result;

	const ElementDeclaration * dec = ElementDictionary::GetDeclaration(token.name);
	ParameterIndex maxArgIndex = kNullParameterIndex;
	auto lastMapping = childArgumentMapping.rbegin();
	if (lastMapping != childArgumentMapping.rend())
	{
		maxArgIndex = lastMapping->first;
	}
	ParameterIndex minParamIndex = dec->GetMinParameterIndex();
	ParameterIndex maxParamIndex = dec->GetMaxParameterIndex();

	for (ParameterIndex index { maxArgIndex.value };
		index.value > minParamIndex.value;
		index.value--)
	{
		// this could runtime assert
		ElementParameter param = dec->GetParameter(index).GetValue();
		if (!param.permutable && index.value != maxArgIndex.value)
		{
			// don't have to worry about any parameters before the most recent one if they aren't permutable
			break;
		}
		if (childArgumentMapping.count(index) == 0
			|| param.repeatable)
		{
			result.AddValidNextArg(nextTokenType, index, param.types);
		}
		if (childArgumentMapping.count(index) == 0
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
		// this could runtime assert
		ElementParameter param = dec->GetParameter(index).GetValue();

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

ElementNode * Parser::GetRightmostElement()
{
	ElementNode * current = &root;
	// we must have at least one non left parameter child
	// and there can be at most one left parameter
	while (
		(current->children.size() == 1
			&& current->childArgumentMapping.count(kLeftParameterIndex) == 0)
		|| current->children.size() > 1)
	{
		current = &current->children.back();
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
	const ElementDeclaration * declaration)
{
	potential.rightSideTypesForLeftParameter =
		potential.rightSideTypesForLeftParameter | node->token.type;

	if (!foundValidLocation
		&& declaration != nullptr
		&& declaration->HasLeftParamterMatching(node->token.type))
	{
		foundValidLocation = true;
		tokenLocation.e = node;
		tokenLocation.isLeftParam = true;
	}
}

Parser::ASTWalkResult Parser::WalkAST(
	ElementToken * nextToken /* = nullptr */,
	bool breakOnFoundLocation /* = false */)
{
	ElementType nextTokenType = 
		nextToken == nullptr
		? ElementType { 0 }
		: nextToken->type;
	const ElementDeclaration * nextDec =
		nextToken == nullptr
		? nullptr
		: ElementDictionary::GetDeclaration(nextToken->name);

	ASTWalkResult astWalkResult;
	if (nextToken != nullptr)
	{
		astWalkResult.walkedWith = *nextToken;
	}

	ElementNode * e = nullptr;
	for(e = GetRightmostElement(); e != nullptr; e = e->parent)
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

	if (e == nullptr)
	{
		// we walked the full tree without breaking
		// which means all nodes have allParametersMet
		astWalkResult.completeStatement = true;
	}

	return astWalkResult;
}

ErrorOr<Success> Parser::Append(ElementToken nextToken)
{
	// returning errors can be destructive here

	if (mostRecentWalkResult.get() == nullptr
		|| !(mostRecentWalkResult->walkedWith.name == nextToken.name))
	{
		*mostRecentWalkResult = WalkAST(&nextToken, true);
	}
	if (!mostRecentWalkResult->foundValidLocation)
	{
		return Error("Could not find location for new token");
	}

	stream.push_back(nextToken);
	ElementNode nextNode {nextToken, ElementIndex{stream.size() - 1}};

	auto & location = mostRecentWalkResult->tokenLocation;
	ElementNode * e = location.e;

	if (location.isLeftParam)
	{
		ElementNode * parent = e->parent;
		CHECK_RETURN(nextNode.Add(*e, kLeftParameterIndex));
		ParameterIndex argIndex = parent->RemoveLastChild();
		CHECK_RETURN(parent->Add(nextNode, argIndex));
	}
	else if (location.argRequiresImpliedNode)
	{
		ElementNode implied {
			ImpliedNodeOptions::acceptedArgTypes.at(nextToken.type),
			kNullElementIndex };
		CHECK_RETURN(implied.Add(nextNode));
		CHECK_RETURN(location.e->Add(implied, location.argIndex));
	}
	else
	{
		CHECK_RETURN(location.e->Add(nextNode, location.argIndex));
	}

	mostRecentWalkResult.reset();

	return Success();
}


Parser::NextTokenCriteria Parser::GetNextTokenCriteria()
{
	if (mostRecentWalkResult.get() == nullptr
		|| mostRecentWalkResult->walkedWith.type != ElementType{0})
	{
		*mostRecentWalkResult = WalkAST();
	}

	return mostRecentWalkResult->potential;
}

} // namespace Command
