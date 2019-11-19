
#ifndef BRUSHLINK_INTERPRETER_HPP
#define BRUSHLINK_INTERPRETER_HPP

#include "ElementTypes.hpp"

namespace Command
{

struct Interpreter
{
	static ErrorOr<Value> Evaluate(
		const CommandContext & context
		const ElementNode & node);

	template<typename TValue>
	static ErrorOr<TValue> EvaluateParameterAs(
		const CommandContext & context
		const ElementNode & node,
		ParameterIndex index)
	{

	}
}

#define EVALUATE_PARAMETER_TYPE_DECLARATION(TValue)
ErrorOr<TValue> EvaluateParameterAs<TValue>( \
	const CommandContext & context \
		const ElementNode & node, \
		ParameterIndex index); \
ErrorOr<Repeatable<TValue> > EvaluateParameterAs<TValue>( \
	const CommandContext & context \
		const ElementNode & node, \
		ParameterIndex index); \ 
ErrorOr<Optional<TValue> > EvaluateParameterAs<TValue>( \
	const CommandContext & context \
		const ElementNode & node, \
		ParameterIndex index); \
ErrorOr<OptionalRepeatable<TValue> > EvaluateParameterAs<TValue>( \
	const CommandContext & context \
		const ElementNode & node, \
		ParameterIndex index);

EVALUATE_PARAMETER_TYPE_DECLARATION()

} // namespace Command


#endif // BRUSHLINK_INTERPRETER_HPP
