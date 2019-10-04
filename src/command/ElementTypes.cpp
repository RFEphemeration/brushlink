#include "ElementTypes.hpp"
#include "ReflectionDeclare.h"
#include "ReflectionBasics.h"

using namespace Farb;

namespace Command
{

const ElementToken ImpliedNodeOptions::selectorToken { ElementName{"Selector"}, ElementType::Selector };
const ElementToken ImpliedNodeOptions::locationToken { ElementName{"Location"}, ElementType::Location };

// accepted arg type -> token to use for node
const std::unordered_map<ElementType::Enum, ElementToken> ImpliedNodeOptions::acceptedArgTypes
{
	{ ElementType::Set, selectorToken },
	{ ElementType::Group_Size, selectorToken },
	{ ElementType::Filter, selectorToken },
	{ ElementType::Superlative, selectorToken },

	{ ElementType::Point, locationToken },
	{ ElementType::Line, locationToken },
	{ ElementType::Area, locationToken }
};

// parameter type -> arg types
const std::unordered_map<ElementType::Enum, ElementType::Enum> ImpliedNodeOptions::potentialParamTypes
{
	{
		ElementType::Selector,

		ElementType::Set
		| ElementType::Group_Size
		| ElementType::Filter
		| ElementType::Superlative
	},
	{
		ElementType::Location,

		ElementType::Point
		| ElementType::Line
		| ElementType::Area
	}
};

std::pair<ElementName, ElementDeclaration> Decl(const ElementDeclaration& decl)
{
	return {decl.name, decl};
}

using namespace ElementType;

const std::map<ElementName, ElementDeclaration> ElementDictionary::declarations
{

	Decl({
		"Attack", Action,
		{
			Selector,
			// rmf todo, default left with implied...
			// and default tree not just single element
			// and merge default children of implied
			{ "Current_Selection", Set }
		},
		{
			{ Selector }
		}
	}),
	{ "Enemies",
		{ "Enemies", Set }
	},
	{ "Within_Range",
		{ "Within_Range", Filter,
			{
				{ Number, { "Zero", Number} }
			}
		}
	},
	{ "Zero",
		{ "Zero", Number }
	},
	{ "One",
		{ "One", Number }
	},
	{ "Selector",
		{ "Selector", Selector,
			{
				{ Set, true },
				{ Group_Size, true },
				{ Filter, true },
				{ Superlative, true },
			}
		}
	}
};

ElementToken::ElementToken(ElementName name)
	: name(name)
	, type(ElementDictionary::GetDeclaration(name)->types)
{ }


bool ElementDeclaration::HasLeftParamterMatching(ElementType::Enum type) const
{
	if (left_parameter.get() == nullptr) return false;
	return (type & left_parameter->types) != kNullElementType;
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

int ElementNode::GetArgCountForParam(ParameterIndex index) const
{
	int count = 0;

	for (auto pair : children)
	{
		if (pair.first == index) count++;
	}

	return count;
}

std::string ElementNode::GetPrintString(const ElementNode & e, std::string indentation, ParameterIndex argIndex)
{
	std::string printString = indentation + e.token.name.value;
	bool annotations = false;
	if (argIndex != kNullParameterIndex)
	{
		printString += " (";
		// todo, left param?
		printString += "arg " + Reflection::ToString(argIndex.value);
		annotations = true;
	}
	if (e.streamIndex == kNullElementIndex)
	{
		printString += ", implied";
		annotations = true;
	}
	if (annotations)
	{
		printString += ")";
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
ElementNode::ArgAndParamWalkResult ElementNode::WalkArgsAndParams(ElementType::Enum nextTokenType /* = kNullElementType */) const
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
		int argCount = GetArgCountForParam(index);
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

ElementNode * Parser::GetRightmostElement()
{
	ElementNode * current = root.get();
	if (current == nullptr)
	{
		return nullptr;
	}
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

ErrorOr<Success> Parser::Append(ElementToken nextToken)
{
	if (root.get() == nullptr)
	{
		stream.push_back(nextToken);
		root = value_ptr<ElementNode>{ElementNode{nextToken, ElementIndex{0}}};
		return Success();
	}

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


Parser::NextTokenCriteria Parser::GetNextTokenCriteria()
{
	if (mostRecentWalkResult.get() == nullptr
		|| mostRecentWalkResult->walkedWith.type != kNullElementType)
	{
		*mostRecentWalkResult = WalkAST();
	}

	return mostRecentWalkResult->potential;
}

} // namespace Command
