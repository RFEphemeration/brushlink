#ifndef COMMAND_ELEMENT_HPP
#define COMMAND_ELEMENT_HPP

#include "CommandParameter.hpp"
#include "ErrorOr.hpp"

namespace Command
{

struct CommandElement
{
	const ElementType type;
	// todo: think more about left parameter here
	// out of scope idea: left parameter OneOf causing dependent type.
	const std::unique_ptr<CommandParameter> left_parameter;
	const std::vector<std::unique_ptr<CommandParameter> > parameters;

	CommandElement(ElementType type,
		std::unique_ptr<CommandParameter> left_parameter,
		std::vector<CommandParameter> parameters)
		: type(type)
		, left_parameter(left_parameter)
		, parameters(parameters)
	{ }

	CommandElement(ElementType type,
		std::vector<CommandParameter> parameters)
		: type(type)
		, left_parameter(nullptr)
		, parameters(parameters)
	{ }

	virtual std::unique_ptr<CommandElement> DeepCopy() = 0;

	ElementType Type() { return type; }

	// these functions follow down the tree

	// not going to bother with keeping track of index locations
	// though I could consider an array of pointers to Parameters
	// but that seems a little precarious
	// I also considered paths through the element arguments
	// but that seems messier / more annoying than traversing twice

	// this map value is the number of occurances
	// so that we can take skip count into consideration
	// @Incomplete: decide skip behavior, I am currently working
	// on the assumption that it is only used for shared types
	Map<ElementType, int> GetAllowedArgumentTypes();

	ErrorOr<bool> AppendArgument(std::unique_ptr<CommandElement> * argument, int &skip_count);

	// what are these functions for if not to assist with
	// building the command tree?

	int ParameterCount() { return parameters.size(); }

	std::set<ElementType> ParameterAllowedTypes(int index);

	bool AddArgument(int index, CommandElement * argument);

	bool ParametersSatisfied();

	template<typename T>
	ErrorOr<T> EvaluateAs(const CommandContext & context)
	{
		Value value = CHECK_RETURN(Evaluate(context));
		if (!std::holds_alternative<T>(value))
		{
			return Error("Element is of unexpected type");
		}
		return std::get<T>(value);
	}

	virtual ErrorOr<Value> Evaluate(const CommandContext & context) = 0;
}

template<typename TVal>
struct Literal : CommandElement
{
	TVal value;

	Literal(ElemetType type, TVal value)
		: CommandElement(type, {})
		, value(value)
	{ }


	std::unique_ptr<CommandElement> DeepCopy() override
	{
		// no need to copy parameters because there are none
		return new Literal(type, value);
	}

	ErrorOr<Value> Evaluate(const CommandContext & context) override
	{
		return value;
	}
}

template<typename TVal>
std::unique_ptr<ElementDefinition> MakeLiteral(
	TVal value)
{
	return new Literal<TVal>{ GetElementType<TVal>(), value };
}

template<typename TRet, typename TArgs...>
struct ContextFunction : CommandElement
{
	ErrorOr<TRet> (CommandContext::*func)(TArgs...);

	ContextFunction(ElementType type,
		ErrorOr<TRet> (CommandContext::*func)(TArgs...),
		std::vector<std::unique_ptr<CommandParameter> > params)
		: CommandElement(type, params)
		, func(func)
	{
		if (sizeof...(TArgs) != params.size())
		{
			// todo: error here
		}
	}

	std::unique_ptr<CommandElement> DeepCopy() override
	{
		std::vector<std::unique_ptr<CommandParameter> > params_copy;
		for (auto param : parameters)
		{
			params_copy.append(param->DeepCopy());
		}
		auto * copy = new ContextFunction(type, func, params_copy);
		return copy;
	}

	ErrorOr<Value> Evaluate(CommandContext & context) override
	{
		if constexpr(sizeof...(TArgs) == 0)
		{
			return CHECK_RETURN((context->*func)())
		}
		else if constexpr(sizeof...(TArgs) == 1)
		{
			auto one = CHECK_RETURN(
				parameters[0]->EvaluateAs<NthTypeOf<0, TArgs...> >(context));
			return CHECK_RETURN((context->*func)(one));
		}
		else if constexpr(sizeof...(TArgs) == 2)
		{
			auto one = CHECK_RETURN(
				parameters[0]->EvaluateAs<NthTypeOf<0, TArgs...> >(context));
			auto two = CHECK_RETURN(
				parameters[1]->EvaluateAs<NthTypeOf<1, TArgs...> >(context));
			return CHECK_RETURN((context->*func)(one, two));
		}
		else if constexpr(sizeof...(TArgs) == 3)
		{
			auto one = CHECK_RETURN(
				parameters[0]->EvaluateAs<NthTypeOf<0, TArgs...> >(context));
			auto two = CHECK_RETURN(
				parameters[1]->EvaluateAs<NthTypeOf<1, TArgs...> >(context));
			auto three = CHECK_RETURN(
				parameters[2]->EvaluateAs<NthTypeOf<2, TArgs...> >(context));
			return CHECK_RETURN((context->*func)(one, two, three));
		}
		else
		{
			static_assert(false, "Add more parameters to ContextFunction");
		}
	}
}

template<typename TRet, typename ... TArgs>
std::unique_ptr<ElementDefinition> MakeContextFunction(
	ElementType type,
	ErrorOr<TRet> (CommandContext::*func)(TArgs...),
	std::vector<CommandParameter> params)
{
	return new ContextFunction{ type, func, params };
}

template<typename TRet, typename TArgs...>
struct ContextFunctionWithActors : CommandElement
{
	ErrorOr<TRet> (CommandContext::*func)(TArgs...);

	ContextFunction(ElementType type,
		ErrorOr<TRet> (CommandContext::*func)(TArgs...),
		std::vector<CommandParameter> params)
		: CommandElement(type, params)
		, func(func)
	{
		if (sizeof...(TArgs) != params.size())
		{
			// todo: error here
		}
	}


	std::unique_ptr<CommandElement> DeepCopy() override
	{
		std::vector<std::unique_ptr<CommandParameter> > params_copy;
		for (auto param : parameters)
		{
			params_copy.append(param->DeepCopy());
		}
		auto * copy = new ContextFunctionWithActors(type, func, params_copy);
		return copy;
	}

	ErrorOr<Value> Evaluate(CommandContext & context) override
	{
		if constexpr(sizeof...(TArgs) == 0)
		{
			static_assert(false, "ContextFunctionWithActors is expected to have at least 1 parameter for the actors");
		}
		UnitGroup actors = CHECK_RETURN(
				parameters[0]->EvaluateAs<UnitGroup >(context));
		context.PushActors(actors);
		ErrorOr<TRet> result;
		if constexpr(sizeof...(TArgs) == 1)
		{
			result = (context->*func)(actors);
		}
		else if constexpr(sizeof...(TArgs) == 2)
		{
			auto two = parameters[1]->EvaluateAs<NthTypeOf<1, TArgs...> >(context);
			if (two.IsError())
			{
				result = two.GetError();
			}
			else
			{	result = (context->*func)(actors, two.GetValue());
		}
		else if constexpr(sizeof...(TArgs) == 3)
		{
			auto two = parameters[1]->EvaluateAs<NthTypeOf<1, TArgs...> >(context);
			auto three = parameters[2]->EvaluateAs<NthTypeOf<2, TArgs...> >(context);
			if (two.IsError())
			{
				result = two.GetError();
			}
			else if (three.IsError())
			{
				result = three.GetError();
			}
			else
			{
				result = (context->*func)(actors, two.GetValue(), three.GetValue());
			}
		}
		else
		{
			static_assert(false, "Add more parameters to ContextFunctionWithActors");
		}
		context.PopActors();
		return CHECK_RETURN(result);
	}
}

template<typename TRet, typename ... TArgs>
std::unique_ptr<ElementDefinition> MakeContextAction(
	ElementType type,
	ErrorOr<TRet> (CommandContext::*func)(TArgs...),
	std::vector<CommandParameter> params)
{
	return new ContextFunctionWithActors{ type, func, params };
}

// used for Command, maybe nothing else
struct EmptyCommandElement : CommandElement
{
	EmptyCommandElement(ElementType type,
		std::vector<CommandParameter> params)
		: CommandElement(type, params)
	{ }

	std::unique_ptr<CommandElement> DeepCopy() override;

	ErrorOr<Value> Evaluate(CommandContext & context) override;

}

struct SelectorCommandElement : CommandElement
{
	/*
	SelectorCommandElement()
		: CommandElement(ElementType.Selector, {
			Param(ElementType.Set,
				OccurrenceFlags.Optional),
			Param(ElementType.Filter,
				OccurrenceFlags.Optional & OccurrenceFlags.Repeatable),
			Param(ElementType.GroupSize,
				OccurrenceFlags.Optional),
			Param(ElementType.Superlative,
				OccurrenceFlags.Optional)
		})
	{ }
	*/

	SelectorCommandElement(std::vector<CommandParameter> params)
		: CommandElement(ElementType.selector, params)
	{ }

public:
	std::unique_ptr<CommandElement> DeepCopy() override;

	ErrorOr<Value> Evaluate(CommandContext & context) override;
}

} // namespace Command

#endif // COMMAND_ELEMENT_HPP