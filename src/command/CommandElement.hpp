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
	const std::vector<CommandParameter> parameters;

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

	ElementType Type() { return type; }

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
		: CommandElement(type, nullptr, {})
		, value(value)
	{ }

	ErrorOr<Value> Evaluate(const CommandContext & context)
	{
		return value;
	}
}

struct Select : CommandElement
{
	CommandElement * actors;

	Select()
		: CommandElement (
			ElementType.Action,
			// @Feature default sub elements like Selector_Base "Allies"
			{new ParamSingleRequired(ElementType.Selector)})
	{}

	ErrorOr<Value> Evaluate(CommandContext & context) override;
}

struct Move : CommandElement
{
	CommandElement * actors;
	CommandElement * location;

	Move()
		: CommandElement(
			ElementType.Action,
			{
				new ParamSingleRequired(ElementType.Selector),
				new ParamSingleRequired(ElementType.Location)
			})
	{ }

	ErrorOr<Value> Evaluate(CommandContext & context) override;
}

template<typename TRet, typename TArgs...>
struct ContextFunction : CommandElement
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

	ErrorOr<Value> Evaluate(CommandContext & context) override
	{
		if constexpr(sizeof...(TArgs) == 0)
		{ 
			(context->*func)()
		}
		else if constexpr(sizeof...(TArgs) == 1)
		{
			auto one = CHECK_RETURN(
				parameters[0]->EvaluateAs<NthTypeOf<0, TArgs...> >(context));
			(context->*func)(one);
		}
		else if constexpr(sizeof...(TArgs) == 2)
		{
			auto one = CHECK_RETURN(
				parameters[0]->EvaluateAs<NthTypeOf<0, TArgs...> >(context));
			auto two = CHECK_RETURN(
				parameters[1]->EvaluateAs<NthTypeOf<1, TArgs...> >(context));
			(context->*func)(one, two);
		}
		else if constexpr(sizeof...(TArgs) == 3)
		{
			auto one = CHECK_RETURN(
				parameters[0]->EvaluateAs<NthTypeOf<0, TArgs...> >(context));
			auto two = CHECK_RETURN(
				parameters[1]->EvaluateAs<NthTypeOf<1, TArgs...> >(context));
			auto three = CHECK_RETURN(
				parameters[2]->EvaluateAs<NthTypeOf<2, TArgs...> >(context));
			(context->*func)(one, two, three);
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

struct CurrentSelection : CommandElement
{
	CurrentSelection()
		: CommandElement(ElementType.Selector_Base, {})
	{}

	ErrorOr<Value> Evaluate(CommandContext & context) override
	{
		return context.CurrentSelection();
	}
}

} // namespace Command

#endif // COMMAND_ELEMENT_HPP