#ifndef COMMAND_ELEMENT_HPP
#define COMMAND_ELEMENT_HPP

#include "CommandParameter.hpp"
#include "ErrorOr.hpp"
#include "MapReduce.hpp"

namespace Command
{

// these different options are used during Undo stacks
// to remove implied elements at the correct time
enum class Implicit
{
	None,
	Child,
	Parent
};

struct CommandElement
{
	// @Incomplete make name const and pass through constructor chain
	ElementName name;
	// @Incomplete probably should do this with implied, too
	Implicit implicit;
	const ElementType::Enum type;
	// todo: think more about left parameter here
	// out of scope idea: left parameter OneOf causing dependent type.
	const value_ptr<CommandParameter> left_parameter;
	const std::vector<value_ptr<CommandParameter> > parameters;

	// @Cleanup this is an awkward member
	// should probably just point to parent and call functions
	value_ptr<CommandElement> * location_in_parent;

	CommandElement(ElementType::Enum type,
		value_ptr<CommandParameter> left_parameter,
		std::vector<value_ptr<CommandParameter> > parameters)
		: implicit(Implicit::None)
		, type(type)
		, left_parameter(std::move(left_parameter))
		, parameters(parameters)
	{ }

	CommandElement(ElementType::Enum type,
		std::vector<value_ptr<CommandParameter> > parameters)
		: implicit(Implicit::None)
		, type(type)
		, left_parameter(nullptr)
		, parameters(parameters)
	{ }

	CommandElement(const CommandElement & other)
		: name(other.name)
		, implicit(other.implicit)
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
	// pair is left param, right params
	void GetAllowedArgumentTypes(AllowedTypes & allowed);

	ErrorOr<bool> AppendArgument(CommandContext & context, value_ptr<CommandElement>&& next, int &skip_count);

	// what are these functions for if not to assist with
	// building the command tree?

	int ParameterCount() { return parameters.size(); }

	bool IsExplicitOrHasExplicitChild();

	// return value is whether to remove this element, also
	ErrorOr<bool> RemoveLastExplicitElement();

	bool ParametersSatisfied();

	std::string GetPrintString(std::string line_prefix);

	template<typename T>
	ErrorOr<T> EvaluateAs(CommandContext & context)
	{
		// Value is used for OneOf
		if constexpr(std::is_same<T, Value>::value)
		{
			return Evaluate(context);
		}
		// Location is a special case OneOf with a specific subset of Value types
		else if constexpr(std::is_same<T,Location>::value)
		{
			Value value = CHECK_RETURN(Evaluate(context));
			return std::visit(
				[](auto&& arg) -> ErrorOr<Location>
				{
					// how to check if this cast is valid? we could enumerate
					// Point, Line, Direction, Area but I'd rather not have to
					return Location{arg};
				},
				value
			);
		}
		else
		{
			Value value = CHECK_RETURN(Evaluate(context));
			if (!std::holds_alternative<T>(value))
			{
				return Error("Element is of unexpected type");
			}
			return std::get<T>(value);
		}
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
	TVal value,
	ElementName name = "")
{
	auto * element = new Literal<TVal>{ GetElementType<TVal>(), value };
	if (name.value != "")
	{
		element->name = name;
	}
	return element;
}

template<typename TRet, typename ... TArgs>
struct ContextFunction : CommandElement
{
	ErrorOr<TRet> (CommandContext::*func)(TArgs...);

	ContextFunction(ElementType::Enum type,
		ErrorOr<TRet> (CommandContext::*func)(TArgs...),
		value_ptr<CommandParameter>&& left_parameter,
		std::vector<value_ptr<CommandParameter> > params)
		: CommandElement(type, left_parameter, params)
		, func(func)
	{ }

	ContextFunction(ElementType::Enum type,
		ErrorOr<TRet> (CommandContext::*func)(TArgs...),
		std::vector<value_ptr<CommandParameter> > params)
		: CommandElement(type, params)
		, func(func)
	{
		if (sizeof...(TArgs) != params.size())
		{
			// todo: error here, also consider checking types
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

		CommandParameter* params[sizeof...(TArgs)];
		int left_offset = 0;
		if (left_parameter != nullptr)
		{
			left_offset = 1;
			params[0] = left_parameter.get();
		}
		for (int i = left_offset; i < sizeof...(TArgs); i++)
		{
			params[i] = parameters[i-left_offset].get();
		}

		if constexpr(sizeof...(TArgs) == 1)
		{
			auto one = params[0]->template EvaluateAs<NthTypeOf<0, TArgs...> >(context);
			if (one.IsError()) return one.GetError();
			return Value{CHECK_RETURN((context.*func)(one.GetValue()))};
		}
		else if constexpr(sizeof...(TArgs) == 2)
		{
			auto one = params[0]->template EvaluateAs<NthTypeOf<0, TArgs...> >(context);
			auto two = params[1]->template EvaluateAs<NthTypeOf<1, TArgs...> >(context);
			if (one.IsError()) return one.GetError();
			if (two.IsError()) return two.GetError();
			return Value{CHECK_RETURN((context.*func)(
				one.GetValue(),
				two.GetValue()))};
		}
		else if constexpr(sizeof...(TArgs) == 3)
		{
			auto one = params[0]->template EvaluateAs<NthTypeOf<0, TArgs...> >(context);
			auto two = params[1]->template EvaluateAs<NthTypeOf<1, TArgs...> >(context);
			auto three = params[2]->template EvaluateAs<NthTypeOf<2, TArgs...> >(context);
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
value_ptr<CommandElement> MakeContextFunction(
	ElementType::Enum type,
	ErrorOr<TRet> (CommandContext::*func)(TArgs...),
	value_ptr<CommandParameter>&& left_parameter,
	std::vector<value_ptr<CommandParameter>> params)
{
	return new ContextFunction{ type, func, std::move(left_parameter), params };
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
