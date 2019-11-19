#ifndef BRUSHLINK_COMMAND_PARSER_H
#define BRUSHLINK_COMMAND_PARSER_H

#include "ElementTypes.hpp"

namespace Command
{

// could implied nodes also be used for parameter_references?
// since their return type is context dependent?
// or should there be different types of parameter_reference nodes?
struct ImpliedNodeOptions
{
	static const ElementToken actionCastToken;
	static const ElementToken selectorToken;
	static const ElementToken locationToken;

	// accepted arg type -> token to use for node
	static const std::unordered_map<ElementType::Enum, ElementToken> acceptedArgTypes;

	// parameter type -> arg types
	static const std::unordered_map<ElementType::Enum, ElementType::Enum> potentialParamTypes;
};

// For an AST, ElementIndexes are in relationship to parent
struct ElementNode
{
	// kNullElementIndex (-1) means you are not in the stream, i.e. implied or default
	ElementIndex streamIndex = kNullElementIndex;
	ElementToken token;

	// in order of appending, aka streamIndex
	// maybe used owned_ptr?
	std::vector<std::pair<ParameterIndex, ElementNode> > children;

	ElementNode* parent = nullptr;

	ElementNode(ElementToken token, ElementIndex streamIndex)
		: token(token)
		, streamIndex(streamIndex)
		, parent(nullptr)
	{ }

	static bool Equal(const ElementNode & a, const ElementNode & b);

	static std::string GetPrintString(const ElementNode & e, std::string indentation = "", ParameterIndex argIndex = kNullParameterIndex);

	int GetArgCountForParam(ParameterIndex index, bool excludeRightmost = false) const;

	ErrorOr<ElementNode *> Add(ElementNode child, ParameterIndex argIndex = kNullParameterIndex);

	ParameterIndex RemoveLastChild();

	void FillDefaultArguments();

	void UpdateChildrenSetParent();

	// these are not recursive, don't guarantee arguments have ParametersMet
	// these three should only be used when you only need one of them
	// if you need two or more, you should call WalkArgsAndParams
	// since these are just wrappers around that
	bool ParametersMet() const;
	ElementType::Enum GetValidNextArgTypes() const;
	ElementType::Enum GetValidNextArgsWithImpliedNodes() const;
	ErrorOr<ParameterIndex> GetArgIndexForNextToken(ElementToken token) const;

	ElementType::Enum GetValidNextArgTypesReplacingRightmost() const;

	struct ArgAndParamWalkResult
	{
		bool allParametersMet { true };
		ElementType::Enum validNextArgs = kNullElementType;
		ElementType::Enum validNextArgsWithImpliedNodes = kNullElementType;
		ParameterIndex firstArgIndexForNextToken { kNullParameterIndex };
		bool firstArgRequiresImpliedNode { false };

		void AddValidNextArg(
			ElementType::Enum nextTokenType,
			ParameterIndex index,
			ElementType::Enum types);
	};

	ArgAndParamWalkResult WalkArgsAndParams(
		ElementType::Enum nextTokenType = kNullElementType,
		bool excludeRightmost = false) const;
};

// can this really be evaluated seperately?
// or are there location/context dependent criteria that won't work?
struct NextTokenCriteria
{
	ElementType::Enum validNextArgs = kNullElementType;
	ElementType::Enum rightSideTypesForLeftParameter = kNullElementType;
	// first = left param type, second = allowed element types
	std::vector<std::pair<ElementType::Enum, ElementType::Enum> > rightSideTypesForMismatchedLeftParameter;
};

struct Parser
{
	struct ASTWalkResult
	{
		ElementToken walkedWith {ElementName{""}, kNullElementType};

		bool completeStatement { false };
		bool foundValidLocation { false };

		// if we found a valid location, this might be incomplete
		NextTokenCriteria potential;

		struct
		{
			// this pointer will only be valid until we update the tree
			// after which it will be meaningless/dangling
			ElementNode * e { nullptr };
			bool isLeftParam { false };

			// nothing past here is used for left params
			ParameterIndex argIndex { kNullParameterIndex };
			bool argRequiresImpliedNode { false };
		} tokenLocation;

		void AddNodeWalkResult(
			ElementNode * node,
			ElementNode::ArgAndParamWalkResult nodeWalkResult);

		void AddTypesForPotentialLeftParams(
			ElementNode * node,
			const ElementDeclaration * declaration);
	};

	std::vector<ElementToken> stream;
	ElementNode root{{"Command", ElementType::Command}, kNullElementIndex};

	value_ptr<ASTWalkResult> mostRecentWalkResult;

	static ErrorOr<ElementNode> Parse(std::vector<ElementName> stream, bool commandRoot = true);

	void Reset();

	ErrorOr<Success> Append(ElementToken nextToken);

	NextTokenCriteria GetNextTokenCriteria();

	bool IsComplete();

	void FillDefaultArguments();

	void FillDefaultArguments(ElementNode & current);

private:

	ElementNode * GetRightmostElement();

	ASTWalkResult WalkAST(ElementToken * nextToken = nullptr,
		bool breakOnFoundLocation = false);
};


} // namespace Command

#endif // BRUSHLINK_COMMAND_PARSER_H
