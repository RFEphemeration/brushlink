#ifndef COMMAND_ELEMENT_HPP
#define COMMAND_ELEMENT_HPP

#include "CommandParameter.hpp"
#include "ErrorOr.hpp"
#include "MapReduce.hpp"

namespace Command
{

struct CommandElement
{
	// @Incomplete make name const and pass through constructor chain
	ElementName name;
	// @Incomplete probably should do this with implied, too
	bool implied;
	const ElementType::Enum type;
	// todo: think more about left parameter here
	// out of scope idea: left parameter OneOf causing dependent type.
	const value_ptr<CommandParameter> left_parameter;
	const std::vector<value_ptr<CommandParameter> > parameters;

	CommandElement(ElementType::Enum type,
		value_ptr<CommandParameter> left_parameter,
		std::vector<value_ptr<CommandParameter> > parameters)
		: type(type)
		, left_parameter(std::move(left_parameter))
		, parameters(parameters)
	{ }

	CommandElement(ElementType::Enum type,
		std::vector<value_ptr<CommandParameter> > parameters)
		: type(type)
		, left_parameter(nullptr)
		, parameters(parameters)
	{ }

	CommandElement(const CommandElement & other)
		: name(other.name)
		, type(other.type)
		, left_parameter(other.left_parameter)
		, parameters(other.parameters)
	{ }

	virtual ~CommandElement() = default;

	// this is intended to only be used by value_ptr internals
	virtual CommandElement * clone() const
	{
		return new CommandElement(*this);
	}

	ElementType::Enum Type() { return type; }

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
	Table<ElementType::Enum, int> GetAllowedArgumentTypes();

	ErrorOr<bool> AppendArgument(value_ptr<CommandElement>&& next, int &skip_count);

	// what are these functions for if not to assist with
	// building the command tree?

	int ParameterCount() { return parameters.size(); }

	Set<ElementType::Enum> ParameterAllowedTypes(int index);

	bool ParametersSatisfied();

	std::string GetPrintString(std::string line_prefix);

	template<typename T>
	ErrorOr<T> EvaluateAs(CommandContext & context)
	{
		if constexpr(std::is_same<T, Value>::value)
		{
			return Evaluate(context);
		}
		else
		{
			Value value = CHECK_RETURN(Evaluate(context));
			if (!std::holds_alternative<T>(value))
			{
				/*
				if constexpr(std::is_same<T,Location>::value)
				{
					if (std::holds_alternative<Point>(value))
					{
						return Location{std::get<Point>(value)};
					}
					else if (std::holds_alternative<Line>(value))
					{
						return Location{std::get<Line>(value)};
					}
					else if (std::holds_alternative<Direction>(value))
					{
						return Location{std::get<Direction>(value)};
					}
					else if (std::holds_alternative<Area>(value))
					{
						return Location{std::get<Area>(value)};
					}
				}
				*/
				return Error("Element is of unexpected type");
			}
			return std::get<T>(value);
		}
	}

	virtual ErrorOr<Value> Evaluate(CommandContext & context)
	{
		// this seems like it's a problem.
		// @Incomplete why can't we use abstract classes with value_ptr?
		return Error("Base CommandElement shouldn't be instantiated");
	}
};

template<typename TVal>
struct Literal : CommandElement
{
	TVal value;

	Literal(ElementType::Enum type, TVal value)
		: CommandElement(type, {})
		, value(value)
	{ }

	Literal(const Literal & other)
		: CommandElement(other)
		, value(other.value)
	{ }

	// this is intended to only be used by value_ptr internals
	virtual CommandElement * clone() const override
	{
		return new Literal(*this);
	}

	ErrorOr<Value> Evaluate(CommandContext & context) override
	{
		return Value{value};
	}
};

template<typename TVal>
value_ptr<CommandElement> MakeLiteral(
	TVal value)
{
	return new Literal<TVal>{ GetElementType<TVal>(), value };
}

template<typename TRet, typename ... TArgs>
struct ContextFunction : CommandElement
{
	ErrorOr<TRet> (CommandContext::*func)(TArgs...);

	ContextFunction(ElementType::Enum type,
		ErrorOr<TRet> (CommandContext::*func)(TArgs...),
		std::vector<value_ptr<CommandParameter> > params)
		: CommandElement(type, params)
		, func(func)
	{
		if (sizeof...(TArgs) != params.size())
		{
			// todo: error here
		}
	}

	ContextFunction(const ContextFunction & other)
		: CommandElement(other)
		, func(other.func)
	{ }

	// this is intended to only be used by value_ptr internals
	virtual CommandElement * clone() const override
	{
		return new ContextFunction(*this);
	}

	ErrorOr<Value> Evaluate(CommandContext & context) override
	{
		if constexpr(sizeof...(TArgs) == 0)
		{
			return Value{CHECK_RETURN((context.*func)())};
		}
		else if constexpr(sizeof...(TArgs) == 1)
		{
			auto one = parameters[0]->template EvaluateAs<NthTypeOf<0, TArgs...> >(context);
			if (one.IsError()) return one.GetError();
			return Value{CHECK_RETURN((context.*func)(one.GetValue()))};
		}
		else if constexpr(sizeof...(TArgs) == 2)
		{
			auto one = parameters[0]->template EvaluateAs<NthTypeOf<0, TArgs...> >(context);
			auto two = parameters[1]->template EvaluateAs<NthTypeOf<1, TArgs...> >(context);
			if (one.IsError()) return one.GetError();
			if (two.IsError()) return two.GetError();
			return Value{CHECK_RETURN((context.*func)(
				one.GetValue(),
				two.GetValue()))};
		}
		else if constexpr(sizeof...(TArgs) == 3)
		{
			auto one = parameters[0]->template EvaluateAs<NthTypeOf<0, TArgs...> >(context);
			auto two = parameters[1]->template EvaluateAs<NthTypeOf<1, TArgs...> >(context);
			auto three = parameters[2]->template EvaluateAs<NthTypeOf<2, TArgs...> >(context);
			if (one.IsError()) return one.GetError();
			if (two.IsError()) return two.GetError();
			if (three.IsError()) return three.GetError();
			return Value{CHECK_RETURN((context.*func)(
				one.GetValue(),
				two.GetValue(),
				three.GetValue()))};
		}
	}
};

template<typename TRet, typename ... TArgs>
value_ptr<CommandElement> MakeContextFunction(
	ElementType::Enum type,
	ErrorOr<TRet> (CommandContext::*func)(TArgs...),
	std::vector<value_ptr<CommandParameter>> params)
{
	return new ContextFunction{ type, func, params };
}

template<typename TRet, typename ... TArgs>
struct ContextFunctionWithActors : CommandElement
{
	ErrorOr<TRet> (CommandContext::*func)(TArgs...);

	ContextFunctionWithActors(ElementType::Enum type,
		ErrorOr<TRet> (CommandContext::*func)(TArgs...),
		std::vector<value_ptr<CommandParameter> > params)
		: CommandElement(type, params)
		, func(func)
	{
		if (sizeof...(TArgs) != params.size())
		{
			// todo: error here
		}
	}

	ContextFunctionWithActors(const ContextFunctionWithActors & other)
		: CommandElement(other)
		, func(other.func)
	{ }

	// this is intended to only be used by value_ptr internals
	virtual CommandElement * clone() const override
	{
		return new ContextFunctionWithActors(*this);
	}

	ErrorOr<Value> Evaluate(CommandContext & context) override
	{
		static_assert(sizeof...(TArgs) != 0, "ContextFunctionWithActors is expected to have at least 1 parameter for the actors");

		UnitGroup actors = CHECK_RETURN(
				parameters[0]->template EvaluateAs<UnitGroup>(context));
		context.PushActors(actors);
		ErrorOr<TRet> result{Success{}};
		if constexpr(sizeof...(TArgs) == 1)
		{
			result = (context.*func)(actors);
		}
		else if constexpr(sizeof...(TArgs) == 2)
		{
			auto two = parameters[1]->template EvaluateAs<NthTypeOf<1, TArgs...> >(context);
			if (two.IsError())
			{
				result = two.GetError();
			}
			else
			{
				result = (context.*func)(actors, two.GetValue());
			}
		}
		else if constexpr(sizeof...(TArgs) == 3)
		{
			auto two = parameters[1]->template EvaluateAs<NthTypeOf<1, TArgs...> >(context);
			auto three = parameters[2]->template EvaluateAs<NthTypeOf<2, TArgs...> >(context);
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
				result = (context.*func)(actors, two.GetValue(), three.GetValue());
			}
		}
		static_assert(sizeof...(TArgs) <= 3, "Add more parameters to ContextFunctionWithActors");

		context.PopActors();
		if (result.IsError())
		{
			return result.GetError();
		}
		else
		{
			return Value{result.GetValue()};
		}
	}
};

template<typename TRet, typename ... TArgs>
value_ptr<CommandElement> MakeContextAction(
	ElementType::Enum type,
	ErrorOr<TRet> (CommandContext::*func)(TArgs...),
	std::vector<value_ptr<CommandParameter>> params)
{
	return new ContextFunctionWithActors<TRet, TArgs...>{ type, func, params };
}

// used for Command, maybe nothing else
struct EmptyCommandElement : CommandElement
{
	EmptyCommandElement(ElementType::Enum type,
		std::vector<value_ptr<CommandParameter>> params)
		: CommandElement(type, params)
	{ }

	EmptyCommandElement(const EmptyCommandElement & other)
		: CommandElement(other)
	{ }

	// this is intended to only be used by value_ptr internals
	virtual CommandElement * clone() const override
	{
		return new EmptyCommandElement(*this);
	}

	ErrorOr<Value> Evaluate(CommandContext & context) override;

};

struct SelectorCommandElement : CommandElement
{
	/*
	SelectorCommandElement()
		: CommandElement(ElementType::Selector, {
			Param(ElementType::Set,
				OccurrenceFlags::Optional),
			Param(ElementType::Filter,
				OccurrenceFlags::Optional & OccurrenceFlags::Repeatable),
			Param(ElementType::Group_Size,
				OccurrenceFlags::Optional),
			Param(ElementType::Superlative,
				OccurrenceFlags::Optional)
		})
	{ }
	*/

	SelectorCommandElement(std::vector<value_ptr<CommandParameter>>&& params)
		: CommandElement(ElementType::Selector, params)
	{ }

	SelectorCommandElement(const SelectorCommandElement & other)
		: CommandElement(other)
	{ }

	// this is intended to only be used by value_ptr internals
	virtual CommandElement * clone() const override
	{
		return new SelectorCommandElement(*this);
	}

public:

	ErrorOr<Value> Evaluate(CommandContext & context) override;
};

} // namespace Command

#endif // COMMAND_ELEMENT_HPP
