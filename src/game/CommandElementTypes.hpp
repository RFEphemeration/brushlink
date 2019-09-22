#ifndef BRUSHLINK_COMMAND_TYPES_HPP
#define BRUSHLINK_COMMAND_TYPES_HPP

using namespace Farb;

namespace Command
{

// todo: interactive visualizations


// keep this in line with Value.value
enum class ValueType
{
	Command_Statement

	//Unit
	Unit_Group // are these two the same?

	Number

	Unit_Type
	Ability_Type
	Attribute_Type

	Point
	Line
	Direction
	Area
}

template<typename T>
constexpr ValueType ValueTypeOf()
{
	if constexpr(std::is_same_v<T, CommandStatement>)
	{
		return ValueType::Command_Statement;
	}
	else if constexpr(std::is_same_v<T, UnitGroup>)
	{
		return ValueType::Unit_Group;
	}
	else if constexpr(std::is_same_v<T, Number>)
	{
		return ValueType::Number;
	}
	else if constexpr(std::is_same_v<T, UnitType>)
	{
		return ValueType::Unit_Type;
	}
	else if constexpr(std::is_same_v<T, AbilityType>)
	{
		return ValueType::Ability_Type;
	}
	else if constexpr(std::is_same_v<T, AttributeType>)
	{
		return ValueType::Attribute_Type;
	}
	else if constexpr(std::is_same_v<T, Point>)
	{
		return ValueType::Point;
	}
	else if constexpr(std::is_same_v<T, Line>)
	{
		return ValueType::Line;
	}
	else if constexpr(std::is_same_v<T, Direction>)
	{
		return ValueType::Direction;
	}
	else if constexpr(std::is_same_v<T, Area>)
	{
		return ValueType::Area;
	}
	else
	{
		static_assert(false, "the provided type is not a Command::ValueType");
	}
}


struct Value
{
	ValueType tag;

	// keep this in line with ValueType
	std::variant<
		CommandStatement,
		UnitGroup,
		Number,
		UnitType,
		AbilityType,
		AttributeType,		
		Point,
		Line,
		Direction,
		Area> value;

	template <typename T>
	inline T& As()
	{
		return std::get<T>(value);
	}

	template <typename T>
	inline const T& As() const
	{
		return std::get<T>(value);
	}

	template <typename T>
	inline bool Is() const
	{
		std::holds_alternative<T>(value);
	}
}


struct ElementIDTag
{
	static HString GetName() { return "Command::ElementID"; }
}
using ElementID = NamedType<HString, ElementIDTag>;

// used as tags on elements for applying restrictions/rules
enum class ElementType
{
	Number,
	Selector_Base,
	Selector_Superlative,
	Selector_Generic
}


struct ElementParameter
{
	ValueType type;
	Bool permutable;
	value_ptr<Value> default_value; // if != nullptr, implies optional
	value_ptr<CRef<Element> > default_element; // must not require parameters
	// todo: one_of variants? unit or units, area or point
	// or should those be handled by implicit conversion?
	// could still list them here, in case the conversion functions differ?
	// these might not be a value, but an element with no parameters that returns

	Value GetDefaultValue(CommandContext context) const;

	bool Optional() const;
}

// the index of a paramter in an element
// -1 is left parameter, 0 is first right parameter
struct ParameterIndexTag
{
	static HString GetName() { return "Command::ParameterIndex"; }
}
using ParameterIndex = NamedType<int, ParameterIndexTag>;

const ParameterIndex kLeftParameterIndex{-1};

// the index of an element in a fragment or command
struct ElementIndexTag
{
	static HString GetName() { return "Command::ElementIndex"; }
}
using ElementIndex = NamedType<int, ElementIndexTag>;

const ElementIndex kNullElementIndex{-1};

struct Element
{
	ValueType type; // return type
	value_ptr<ElementParameter> left_parameter; // optional
	std::vector<ElementParameter> right_parameters; // could be length 0

	// errors should be reserved for programming errors I think?
	// how should we handle user facing feedback like selector contains no units?
	virtual ErrorOr<Value> Evaluate(
		CommandContext context,
		std::map<ParameterIndex, Value> arguments) const;

protected:
	ErrorOr<Success> FillDefaultArguments(
		CommandContext context,
		std::map<ParameterIndex,CRef<Value> >& arguments) const;
}

struct ElementAtomDynamic : Element
{
	ErrorOr<Value> (*underlying_function)(
		CommandContext,
		std::map<ParameterIndex, Value>);

	virtual ErrorOr<Value> Evaluate(
		CommandContext context,
		std::map<ParameterIndex, Value> arguments) const override
	{
		CHECK_RETURN(FillDefaultArguments(context, arguments));
		return underlying_function(context, arguments);
	}
}

template<typename TRet, typename ... TArgs>
struct ElementAtom : Element
{
	ErrorOr<TRet> (*underlying_function)(CommandContext, TArgs...);

	ElementAtom(
		ErrorOr<TRet> (*underlying_function)(CommandContext, TArgs...),
		value_ptr<ElementParameter> left_parameter,
		std::vector<ElementParameter> right_parameters)
		: Element(ValueTypeOf<TRet>(), left_parameter, right_parameters)
	{

	}

	// assumes all right parameters, no default values, no permutations
	ElementAtom(
		ErrorOr<TRet> (*underlying_function)(CommandContext, TArgs...))
		: Element(ValueTypeOf<TRet>())
	{
		if constexpr (size_of...(TArgs) > 0)
		{
			PushBackGeneratedParams<TArgs...>();
		}
	}

	virtual ErrorOr<Value> Evaluate(
		CommandContext context,
		std::map<ParameterIndex, Value> arguments) const override
	{
		CHECK_RETURN(FillDefaultArguments(context, arguments));

		auto baseFunctor = FunctionPointer { underlying_function };
		auto funcWithContext = CurriedFunctor { baseFunctor, context };

		TRet value = CHECK_RETURN(ApplyArguments(funcWithContext, arguments));

		return Value(value);
	}

	inline ErrorOr<TRet> ApplyArguments(
		Functor<TRet>& func,
		std::map<ParameterIndex, Value>& arguments) const
	{
		if (!arguments.empty())
		{
			return Error("Something went wrong when currying ElementAtom arguments, even though we started with the correct number, we have some left over");
		}
		return func();
	}

	template<typename TArg, typename ... TRest>
	inline ErrorOr<TRet> ApplyArguments(
		Functor<TRet, TArg, TRest...>& func,
		std::map<ParameterIndex, Value>& arguments) const
	{
		if (arguments.empty())
		{
			return Error("Something went wrong when currying ElementAtom arguments, even though we started with the correct number, we ran out before finishing");
		}
		auto argIter = arguments.begin();
		if (!argIter->second.Is<TArg>())
		{
			return Error("ElementAtom received incorrect argument type for parameter " + argIter.first.value);
		}

		auto curriedFunction = CurriedFunctor { func, argIter->second.As<TArg>() };
		arguments.erase(argIter);

		return ApplyArguments(curriedFunction, arguments);
	}

private:
	template <typename TParam, typename... TRest>
	constexpr void PushBackGeneratedParams()
	{
		right_parameters.push_back(
			ElementParameter{
				ValueTypeOf<TParam>,
				false,
				nullptr
			});

		if constexpr (size_of...(TRest) == 0)
		{
			return;
		}
		else
		{
			PushBackGeneratedParams<TRest>();
		}
	}
}

// is there such a thing as an element literal?
// even numbers take an optional left_param for multiple digits
// would probably be useful for writing words in scripts
struct ElementLiteral : Element
{
	Value value;

	ElementLiteral(Value value)
		: Element(value.type)
		, value(value)
	{ }

	virtual ErrorOr<Value> Evaluate(
		CommandContext context,
		std::map<ParameterIndex, Value> arguments) const override
	{
		return value;
	}
	
}

// only used in the construction of ElementWord objects
// to represent the word parameters, index is relative to the parent
struct ElementReference : Element
{
	ElementReference(const Element & parent, ParameterIndex index)
		: Element(parent.GetParameter(index).type)
		, parameter(index);

	ParameterIndex parameter;
}

// only useful in the context of a FragmentMapping
// since that provides the reference list for the ElementIndex;
struct ElementMapping
{
	const Element& element;
	std::map<ParameterIndex, ElementIndex> arguments;
	std::pair<ElementIndex, ParameterIndex> parent;
		// kNullElementIndex (-1) means you have no parent
}

struct FramentMapping
{
	std::vector<ElementMapping> elementMapping;
	std::vector<ElementIndex> evaluationOrder;

	FragmentMapping(const std::vector<CRef<Element> > & elements);

	void Append(const Element & e, bool evaluateOrder = true);

	// this is recursive descent
	// requires precondition that everything in evaluationOrder so far
	// isn't dependent on the element at the provided index
	void EvaluateOrderFrom(ElementIndex index);

	ErrorOr<Success> EvaluateOrder();

	static ErrorOr<FragmentMapping> CreateMapping(const std::vector<CRef<Element> > &elements)
	{
		return FragmentMapping{elements};
	}
}

// made up from Atoms, Literals, and other Words
struct ElementWord : Element
{
	// TypeInfoAs std::vector<Element&>
	FramentMapping implementation;

	// we can't do recursive descent here because ElementReferences
	// must be resolved in this scope, where they can access this Word's arguments
	// even if they are arguments to other Elements in the implementation
	virtual ErrorOr<Value> Evaluate(
		CommandContext context,
		std::map<ParameterIndex, Value> arguments) const override;

private:
	ErrorOr<Success> PostLoad();
}


struct Action
{
	Target target;
}



struct Point
{

}

struct Line
{

}

} // namespace Command

#endif // BRUSHLINK_COMMAND_TYPES_HPP
