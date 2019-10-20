#include "Parser.h"

#include "ReflectionDeclare.h"
#include "ReflectionBasics.h"
#include "ElementDictionary.h"

using namespace Farb;

namespace Command
{
using namespace ElementType;

const ElementToken ImpliedNodeOptions::actionCastToken { ElementName{"Cast"}, Action };
const ElementToken ImpliedNodeOptions::selectorToken { ElementName{"Selector"}, Selector };
const ElementToken ImpliedNodeOptions::locationToken { ElementName{"Location"}, Location };

// accepted arg type -> token to use for node
const std::unordered_map<ElementType::Enum, ElementToken> ImpliedNodeOptions::acceptedArgTypes
{

	{ Ability_Type, actionCastToken },

	{ Set, selectorToken },
	{ Group_Size, selectorToken },
	{ Filter, selectorToken },
	{ Superlative, selectorToken },

	{ Point, locationToken },
	{ Line, locationToken },
	{ Area, locationToken },

};

// parameter type -> arg types
const std::unordered_map<ElementType::Enum, ElementType::Enum> ImpliedNodeOptions::potentialParamTypes
{
	{
		Selector,

		Set
		| Group_Size
		| Filter
		| Superlative
	},
	{
		Location,

		Point
		| Line
		| Area
	}
};

bool ElementNode::Equal(const ElementNode & a, const ElementNode & b)
{
	bool equal = a.streamIndex == b.streamIndex
		&& a.token.type == b.token.type
		&& a.token.name == b.token.name
		&& a.children.size() == b.children.size();
	if (!equal)
	{
		return false;
	}

	for (int i = 0; i < a.children.size(); i++)
	{
		equal = equal
			&& a.children[i].first == b.children[i].first
			&& Equal(a.children[i].second, b.children[i].second);
	}

	return equal;
}

int ElementNode::GetArgCountForParam(ParameterIndex index, bool excludeRightmost /* = false */) const
{
	int count = 0;

	for (auto pair : children)
	{
		if (pair.first == index) count++;
	}

	if (excludeRightmost
		&& count > 0
		&& children.back().first == index)
	{
		count--;
	}

	return count;
}

std::string ElementNode::GetPrintString(const ElementNode & e, std::string indentation, ParameterIndex argIndex)
{
	std::string printString = indentation + e.token.name.value;
	std::string annotations;
	if (argIndex != kNullParameterIndex)
	{
		// todo, left param?
		annotations += "arg " + Reflection::ToString(argIndex.value);
	}
	if (e.streamIndex == kNullElementIndex)
	{
		if (!annotations.empty())
		{
			annotations += ", ";
		}
		annotations += "implied";
	}
	if (!annotations.empty())
	{
		printString += " (" + annotations + ")";
	}
	printString += "\n";
	indentation += "   ";
	for (const auto & pair : e.children)
	{
		printString += GetPrintString(
			pair.second,
			indentation,
			pair.first);
	}
	return printString;
}

ErrorOr<ElementNode *> ElementNode::Add(ElementNode child, ParameterIndex argIndex)
{
	// this is often passed in after already walking the args and params
	if (argIndex == kNullParameterIndex)
	{
		argIndex = CHECK_RETURN(GetArgIndexForNextToken(child.token));
	}

	child.parent = this;
	children.push_back({argIndex, child});
	UpdateChildrenSetParent();
	return &(children.back().second);
}

ParameterIndex ElementNode::RemoveLastChild()
{	
	if (children.size() == 0)
	{
		return kNullParameterIndex;
	}

	ParameterIndex index = children[children.size() - 1].first;
	children.pop_back();

	return index;
}

void ElementNode::FillDefaultArguments()
{
	const ElementDeclaration * dec = ElementDictionary::GetDeclaration(token.name);

	ParameterIndex minParamIndex = dec->GetMinParameterIndex();
	ParameterIndex maxParamIndex = dec->GetMaxParameterIndex();

	for (ParameterIndex paramIndex = minParamIndex;
		paramIndex.value <= maxParamIndex.value;
		paramIndex.value++)
	{
		ElementParameter param = dec->GetParameter(paramIndex).GetValue();
		if (param.default_value.get() == nullptr)
		{
			// no need to try to fill defaults for parameters that don't have them
			// might as well avoid searching through our arguments
			continue;
		}
		bool existsArgForParam = false;
		for (auto pair : children)
		{
			ParameterIndex argIndex = pair.first;
			if (argIndex == paramIndex)
			{
				existsArgForParam = true;
				break;
			}
		}
		if (!existsArgForParam)
		{
			Add({ { *param.default_value }, kNullElementIndex }, paramIndex);
		}
	}
}

void ElementNode::UpdateChildrenSetParent()
{
	for (auto & pair : children)
	{
		pair.second.parent = this;
		pair.second.UpdateChildrenSetParent();
	}
}

bool ElementNode::ParametersMet() const
{
	return WalkArgsAndParams().allParametersMet;
}

ElementType::Enum ElementNode::GetValidNextArgTypesReplacingRightmost() const
{
	if (children.empty())
	{
		return kNullElementType;
	}
	auto walkResult = WalkArgsAndParams(kNullElementType, true);
	ElementType::Enum allowed = walkResult.validNextArgs
		| walkResult.validNextArgsWithImpliedNodes;
	return allowed;
}

ElementType::Enum ElementNode::GetValidNextArgTypes() const
{
	return WalkArgsAndParams().validNextArgs;
}

ElementType::Enum ElementNode::GetValidNextArgsWithImpliedNodes() const
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

void ElementNode::ArgAndParamWalkResult::AddValidNextArg(ElementType::Enum nextTokenType, ParameterIndex index, ElementType::Enum types)
{
	validNextArgs = validNextArgs | types;

	ElementType::Enum typesWithImplied = kNullElementType;

	for (auto pair : ImpliedNodeOptions::potentialParamTypes)
	{
		if ((pair.first & types) != kNullElementType)
		{
			typesWithImplied = typesWithImplied | pair.second;
		}
	}

	validNextArgsWithImpliedNodes = validNextArgsWithImpliedNodes |  typesWithImplied;

	if ((firstArgIndexForNextToken == kNullParameterIndex
			|| index.value < firstArgIndexForNextToken.value)
		&& ((types & nextTokenType) != kNullElementType
			|| (typesWithImplied & nextTokenType) != kNullElementType))
	{
		if ((types & nextTokenType) != kNullElementType)
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
ElementNode::ArgAndParamWalkResult ElementNode::WalkArgsAndParams(
	ElementType::Enum nextTokenType /* = kNullElementType */,
	bool excludeRightmost /* = false */ ) const
{
	ArgAndParamWalkResult result;

	const ElementDeclaration * dec = ElementDictionary::GetDeclaration(token.name);
	ParameterIndex maxArgIndex = kNullParameterIndex;
	for (auto pair : children)
	{
		if (pair.first > maxArgIndex)
		{
			maxArgIndex = pair.first;
		}
	}

	ParameterIndex minParamIndex = dec->GetMinParameterIndex();
	ParameterIndex maxParamIndex = dec->GetMaxParameterIndex();

	for (ParameterIndex index = maxArgIndex.value;
		index.value >= minParamIndex.value;
		index.value--)
	{
		// this could runtime assert
		ElementParameter param = dec->GetParameter(index).GetValue();
		if (!param.permutable && index.value != maxArgIndex.value)
		{
			// don't have to worry about any parameters before the most recent one if they aren't permutable
			break;
		}
		int argCount = GetArgCountForParam(index, excludeRightmost);
		if (argCount == 0
			|| param.repeatable)
		{
			result.AddValidNextArg(nextTokenType, index, param.types);
		}
		if (argCount == 0
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
	for (ParameterIndex index { std::max(maxArgIndex.value + 1, minParamIndex.value)};
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

		// left parameters are not valid next arguments
		if (index > kLeftParameterIndex)
		{
			result.AddValidNextArg(nextTokenType, index, param.types);
		}

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

void Parser::FillDefaultArguments()
{
	FillDefaultArguments(root);
}

void Parser::FillDefaultArguments(ElementNode & current)
{
	current.FillDefaultArguments();
	for(auto & pair : current.children)
	{
		FillDefaultArguments(pair.second);
	}
}

ElementNode * Parser::GetRightmostElement()
{
	ElementNode * current = &root;

	// we must have at least one non left parameter child
	// and there can be at most one left parameter
	while (
		(current->children.size() == 1
			&& current->GetArgCountForParam(kLeftParameterIndex) == 0)
		|| current->children.size() > 1)
	{
		ElementNode * previous = current;
		current = &(current->children.back().second);
		if (current->parent != previous)
		{
			// todo: log an error here
			assert(false);
		}
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

	ElementType::Enum allowedToReplaceLeft = kNullElementType;

	if (node->parent != nullptr)
	{
		allowedToReplaceLeft = node->parent->GetValidNextArgTypesReplacingRightmost();
		potential.rightSideTypesForMismatchedLeftParameter.push_back({
			node->token.type,
			allowedToReplaceLeft});
	}

	if (!foundValidLocation
		&& declaration != nullptr
		&& declaration->HasLeftParamterMatching(node->token.type)
		&& (allowedToReplaceLeft & declaration->types) != kNullElementType)
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
	ElementType::Enum nextTokenType = 
		nextToken == nullptr
		? kNullElementType
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

	ElementNode * e = GetRightmostElement();
	for(; e != nullptr; e = e->parent)
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

void Parser::Reset()
{
	stream.clear();
	root = {{"Command", ElementType::Command}, kNullElementIndex};
	mostRecentWalkResult.reset();
}

ErrorOr<Success> Parser::Append(ElementToken nextToken)
{
	// returning errors can be destructive here
	if (mostRecentWalkResult.get() == nullptr
		|| !(mostRecentWalkResult->walkedWith.name == nextToken.name))
	{
		mostRecentWalkResult = value_ptr<ASTWalkResult>{WalkAST(&nextToken, true)};
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


NextTokenCriteria Parser::GetNextTokenCriteria()
{
	if (mostRecentWalkResult.get() == nullptr
		|| mostRecentWalkResult->walkedWith.type != kNullElementType)
	{
		mostRecentWalkResult = value_ptr<ASTWalkResult>{WalkAST()};
	}

	return mostRecentWalkResult->potential;
}

bool Parser::IsComplete()
{
	if (mostRecentWalkResult.get() == nullptr
		|| mostRecentWalkResult->walkedWith.type != kNullElementType)
	{
		mostRecentWalkResult = value_ptr<ASTWalkResult>{WalkAST()};
	}

	return mostRecentWalkResult->completeStatement;
}

} // namespace Command
