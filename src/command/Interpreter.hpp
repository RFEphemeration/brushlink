
#ifndef BRUSHLINK_INTERPRETER_HPP
#define BRUSHLINK_INTERPRETER_HPP

#include "ElementTypes.hpp"

namespace Command
{

template<typename TContainer, typename TValue>
struct ArgumentAccumulator
{
	static ErrorOr<Success> Add(
		TContainer & args,
		const TValue & arg)
	{
		if constexpr (std::is_same_v<TContainer, TValue>)
		{
			args = arg;
		}
		else if constexpr(std::is_same_v<TContainer, Optional<TValue>>)
		{
			args = arg;
		}
		else if constexpr(std::is_same_v<TContainer, Repeatable<TValue>>
			|| std::is_same_v<TContainer, OptionalRepeatable<TValue>>)
		{
			args.push_back(arg);
		}
	}
}

struct Interpreter
{
	static inline ErrorOr<Value> Evaluate(
		const CommandContext & context
		const ElementNode & node)
	{
		auto decl = ElementDictionary::GetDeclaration(node.token.name);
		if (decl == nullptr)
		{
			return Error{"Couldn't find declaration for element " + node.token.name};
		}
		if (decl->definition == nullptr)
		{
			return Error{"Couldn't find definition for element " + node.token.name};
		}
		return decl->definition->Evaluate(context, node);
	}

	template<typename TValue>
	static inline ErrorOr<TValue> EvaluateAs(
		const CommandContext & context
		const ElementNode & node)
	{
		auto value = CHECK_RETURN(Evaluate(context, node));
		if (std::holds_alternative<TValue>(value))
		{
			return std::get<TValue>(value);
		}
		return Error{"Value is not of expected type"};
	}

	template<typename TWrapped>
	static ErrorOr<TWrapped> EvaluateParameterAs(
		const CommandContext & context
		const ElementNode & node,
		ParameterIndex index)
	{
		const ElementDeclaration * dec = ElementDictionary::GetDeclaration(node.token.name);
		if (dec == nullptr)
		{
			return Error{"Couldn't find element declaration for " + node.token.name.value};
		}
		auto param = CHECK_RETURN(dec->GetParameter(index));

		std::vector<const ElementNode *> pArgs = node.GetArgumentsForParam(index);
		if (!param.optional && pArgs.size() == 0)
		{
			return Error{"Parameter at index " + index.value + " is required"};
		}
		if (!param.repeatable && pArgs.size() > 1)
		{
			return Error{"Parameter at index " + index.value + " can't have multiple values"};
		}

		TWrapped result;
		using TValue = Underlying<TWrapped>::Type;
		for (auto pArg : pArgs)
		{
			auto value = CHECK_RETURN(EvaluateAs<TValue>(context, *pArg));
			ArgumentAccumulator::Add(result, value);
		}
		return result;
	}

	template<typename TContainer<typename TValue> >
	static ErrorOr<Repeatable<TValue>> EvaluateParameterAs(
		const CommandContext & context
		const ElementNode & node,
		ParameterIndex index)
	{
		auto args = node.GetArgumentsForParam(index);
		if (args.size() == 0)
		{
			return Error{"Parameter at index " + index.value + " requires at least one value as an argument"};
		}
	}

	template<typename TValue>
	static ErrorOr<Optional<TValue>> EvaluateParameterAs(
		const CommandContext & context
		const ElementNode & node,
		ParameterIndex index)
	{
		auto args = node.GetArgumentsForParam(index);
		if (args.size() > 1)
		{
			return Error{"Parameter at index " + index.value + " requires at most one value as an argument"};
		}
		
	}

	template<typename TValue>
	static ErrorOr<OptionalRepeatable<TValue>> EvaluateParameterAs(
		const CommandContext & context
		const ElementNode & node,
		ParameterIndex index)
	{
		auto args = node.GetArgumentsForParam(index);

	}
}

#define EVALUATE_TYPE_DECLARATION(TValue)
ErrorOr<TValue> Interpreter::EvaluateAs<TValue>( \
	const CommandContext & context, \
	const ElementNode & node); \
ErrorOr<TValue> Interpreter::EvaluateParameterAs<TValue>( \
	const CommandContext & context, \
	const ElementNode & node, \
	ParameterIndex index); \
ErrorOr<Repeatable<TValue> > Interpreter::EvaluateParameterAs<TValue>( \
	const CommandContext & context, \
	const ElementNode & node, \
	ParameterIndex index); \ 
ErrorOr<Optional<TValue> > Interpreter::EvaluateParameterAs<TValue>( \
	const CommandContext & context, \
	const ElementNode & node, \
	ParameterIndex index); \
ErrorOr<OptionalRepeatable<TValue> > Interpreter::EvaluateParameterAs<TValue>( \
	const CommandContext & context, \
	const ElementNode & node, \
	ParameterIndex index);

// do this for each value type
EVALUATE_TYPE_DECLARATION()

} // namespace Command


#endif // BRUSHLINK_INTERPRETER_HPP
