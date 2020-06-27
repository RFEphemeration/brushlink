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
	const ElementName name;
	const ElementType::Enum type;
	// todo: think more about left parameter here
	// out of scope idea: left parameter OneOf causing dependent type.
	const value_ptr<CommandParameter> left_parameter;
	const std::vector<value_ptr<CommandParameter> > parameters;

	// these changes depending on contextual use, so they cannot be const
	Implicit implicit;

	// @Cleanup this is an awkward member
	// should probably just point to parent and call functions
	value_ptr<CommandElement> * location_in_parent;

	CommandElement(ElementName name,
		ElementType::Enum type,
		value_ptr<CommandParameter>&& left_parameter,
		std::vector<value_ptr<CommandParameter> >&& parameters,
		Implicit implicit = Implicit::None)
		: name(name)
		, type(type)
		, left_parameter(std::move(left_parameter))
		, parameters(std::move(parameters))
		, implicit(implicit)
		, location_in_parent(nullptr)
	{ }

	CommandElement(ElementName name,
		ElementType::Enum type,
		std::vector<value_ptr<CommandParameter> >&& parameters = {},
		Implicit implicit = Implicit::None)
		: name(name)
		, type(type)
		, left_parameter(nullptr)
		, parameters(std::move(parameters))
		, implicit(implicit)
		, location_in_parent(nullptr)
	{ }

	CommandElement(CommandElement && other)
		: name(other.name)
		, implicit(other.implicit)
		, type(other.type)
		, left_parameter(std::move(other.left_parameter))
		, parameters(std::move(other.parameters))
		, location_in_parent(nullptr)
	{ }

	CommandElement(const CommandElement & other)
		: name(other.name)
		, implicit(other.implicit)
		, type(other.type)
		, left_parameter(other.left_parameter)
		, parameters(other.parameters)
		, location_in_parent(nullptr)
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

	bool IsExplicitOrHasExplicitChild();

	// return value is whether to remove this element, also
	ErrorOr<bool> RemoveLastExplicitElement();

	bool ParametersSatisfied();

	virtual std::string GetPrintString(std::string line_prefix);

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

	Literal(ElementName name, TVal value)
		: CommandElement(name, GetElementType<TVal>())
		, value(value)
	{ }


	Literal(CommandElement&& element, TVal value)
		: CommandElement(element)
		, value(value)
	{
		if (parameters.size() > 0 || left_parameter != nullptr)
		{
			// todo error here
		}
	}

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

template<typename TRet, typename ... TArgs>
struct ContextFunction : CommandElement
{
	ErrorOr<TRet> (CommandContext::*func)(TArgs...);

	ContextFunction(CommandElement&& element,
		ErrorOr<TRet> (CommandContext::*func)(TArgs...))
		: CommandElement(element)
		, func(func)
	{
		if (sizeof...(TArgs) != parameters.size())
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

	template<int N>
	ErrorOr<NthTypeOf<N, TArgs...> > EvaluateParam(CommandParameter * param, CommandContext & context)
	{
		if constexpr (IsSpecialization<NthTypeOf<N, TArgs...>, std::vector>::value)
		{
			return param->template EvaluateAsRepeatable<typename NthTypeOf<N, TArgs...>::value_type>(context);
		}
		else
		{
			return param->template EvaluateAs<NthTypeOf<N, TArgs...> >(context);
		}
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
			auto one = EvaluateParam<0>(params[0], context);
			if (one.IsError()) return one.GetError();
			return Value{CHECK_RETURN((context.*func)(one.GetValue()))};
		}
		else if constexpr(sizeof...(TArgs) == 2)
		{
			auto one = EvaluateParam<0>(params[0], context);
			auto two = EvaluateParam<1>(params[1], context);
			if (one.IsError()) return one.GetError();
			if (two.IsError()) return two.GetError();
			return Value{CHECK_RETURN((context.*func)(
				one.GetValue(),
				two.GetValue()))};
		}
		else if constexpr(sizeof...(TArgs) == 3)
		{
			auto one = EvaluateParam<0>(params[0], context);
			auto two = EvaluateParam<1>(params[1], context);
			auto three = EvaluateParam<2>(params[2], context);
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
struct ContextFunctionWithActors : CommandElement
{
	ErrorOr<TRet> (CommandContext::*func)(TArgs...);

	ContextFunctionWithActors(CommandElement&& element,
		ErrorOr<TRet> (CommandContext::*func)(TArgs...))
		: CommandElement(element)
		, func(func)
	{
		if (sizeof...(TArgs) != parameters.size())
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

// used for Command, maybe nothing else
struct EmptyCommandElement : CommandElement
{

	EmptyCommandElement(CommandElement&& element)
		: CommandElement(element)
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

	SelectorCommandElement(value_ptr<CommandParameter>&& set_param)
		: CommandElement("Selector",
			ElementType::Selector,
			{set_param,
			new ParamRepeatableOptional{ElementType::Filter, ""},
			new ParamSingleOptional{ElementType::Group_Size, ""},
			new ParamSingleOptional{ElementType::Superlative, "SuperlativeRandom"}})
	{ }

	SelectorCommandElement(CommandElement&& element)
		: CommandElement(element)
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

struct NumberLiteralCommandElement : CommandElement
{
	NumberLiteralCommandElement()
		: CommandElement("NumberLiteral", ElementType::Number, {new ParamRepeatableRequired{ElementType::Digit}})
	{ }


	NumberLiteralCommandElement(const NumberLiteralCommandElement & other)
		: CommandElement(other)
	{ }

	// this is intended to only be used by value_ptr internals
	virtual CommandElement * clone() const override
	{
		return new NumberLiteralCommandElement(*this);
	}

	virtual std::string GetPrintString(std::string line_prefix) override
	{
		// @Cleanup this feels pretty gross...
		auto* param = dynamic_cast<ParamRepeatableRequired*>(parameters[0].get());
		if (param == nullptr)
		{
			return "Invalid NumberLiteral parameter";
		}
		int value = 0;
		for (auto & arg : param->arguments)
		{
			auto* literal = dynamic_cast<Literal<Digit>*>(arg.get());
			if (literal == nullptr)
			{
				return "NumberLiteral is expected to only have Literal Digit arguments";
			}
			value = (value * 10) + literal->value.value;
		}
		return line_prefix + std::to_string(value) + "\n";
	}

	ErrorOr<Value> Evaluate(CommandContext & context) override
	{
		auto digits = CHECK_RETURN(parameters[0]->EvaluateAsRepeatable<Digit>(context));
		int value = 0;
		for (auto & digit : digits)
		{
			value = (value * 10) + digit.value;
		}
		return Value{Number{value}};
	}
};

} // namespace Command

#endif // COMMAND_ELEMENT_HPP
