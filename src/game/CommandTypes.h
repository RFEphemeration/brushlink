
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

	Point
	Line
	Direction
	Area
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
		Point,
		Line,
		Direction,
		Area> value;

	template <typename T>
	T& As()
	{
		return std::get<T>(value);
	}

	template <typename T>
	bool Is()
	{
		std::holds_alternative<T>(value);
	}
}


struct ElementParameter
{
	ValueType type;
	Bool permutable;
	value_ptr<Value> default_value; // if != nullptr, implies optional
	// todo: one_of variants? unit or units, area or point
	// or should those be handled by implicit conversion?
	// could still list them here, in case the conversion functions differ?
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

	virtual ErrorOr<Value> Evaluate(
		CommandContext context,
		std::map<ParameterIndex, CRef<Value> > arguments) const;

protected:
	ErrorOr<Success> FillDefaultArguments(
		std::map<ParameterIndex, CRef<Value> >& arguments) const
	{
		if (arguments.count(kLeftParameterIndex) == 0
			&& left_parameter != nullptr)
		{
			if (left_parameter.default_value == nullptr)
			{
				return Error("Missing required left argument for Element");
			}
			arguments.insert(kLeftParameterIndex, cref(*left_parameter.default_value));
		}

		for (int arg = 0; arg < right_parameters.count(); arg++)
		{
			if (arguments.count(arg) > 0)
			{
				continue;
			}
			if (right_parameters[arg].default_value == nullptr)
			{
				return Error("Missing required argument " + arg + " for Element");
			}
			arguments.insert(ParameterIndex(arg), cref(*right_parameters[arg].default_value));
		}

		int desired_argument_count = right_parameters.count();
		desired_argument_count += left_parameter != nullptr ? 1 : 0;

		if (arguments.count() != desired_argument_count)
		{
			return Error("Too many arguments for Element");
		}

		return Success();
	}
}

struct ElementAtomDynamic : Element
{
	Value (*underlying_function)(CommandContext, std::map<ParameterIndex, Value>);

	virtual Value Evaluate(
		CommandContext context,
		std::map<ParameterIndex, Value> arguments) const override
	{
		CHECK_RETURN(FillDefaultArguments(arguments));
		return underlying_function(context, arguments);
	}
}

template<typename TRet, typename ... TArgs>
struct ElementAtom : Element
{
	TRet (*underlying_function)(CommandContext, TArgs...);

	virtual Value Evaluate(
		CommandContext context,
		std::map<ParameterIndex, Value> arguments) const override
	{
		CHECK_RETURN(FillDefaultArguments(arguments));

		if (arguments.count() == 0)
		{
			return Value(underlying_function(context));
		}
		auto baseFunctor = FunctionPointer { underlying_function };
		auto funcWithContext = CurriedFunctor { baseFunctor, context };

		TRet value = ApplyArguments(funcWithContext, arguments);

		return Value(value);
	}

	ErrorOr<TRet> ApplyArguments(
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
	ErrorOr<TRet> ApplyArguments(
		Functor<TRet, TArg, TRest...>& func,
		std::map<ParameterIndex, Value>& arguments) const
	{
		auto argIter = arguments.begin();
		if (!argIter->second.Is<TArg>())
		{
			return Error("ElementAtom received incorrect argument type for parameter " + argIter.first.value);
		}

		auto curriedFunction = CurriedFunctor { func, argIter->second.As<TArg>() };
		arguments.erase(argIter);

		return ApplyArguments(curriedFunction, arguments);
	}

	template<typename TArg, typename ... TRemaining>
	ErrorOr<TRet> InternalEvaluate(CommandContext context,
		std::map<ParameterIndex, Value>& arguments) const
	{
		auto pair = arguments.begin();
		if (!pair->second.Is<TArg>())
		{
			return Error("ElementAtom received incorrect arguments");
		}
		.As<TArg>();
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

	virtual Value Evaluate(
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

	FragmentMapping(const std::vector<CRef<Element> > & elements)
	{
		for(const Element & e : elements)
		{
			Append(e, false);
		}
	}

	void Append(const Element & e, bool evaluateOrder = true)
	{
		ElementIndex eIndex { elementMapping.count() };
		elementMapping.push_back(ElementMapping{e});

		if (e.left_parameter != nullptr)
		{
			int leftIndex = FindAppropriateLeftParameter(e);
			Pair<int, int> parentIndex = parents[leftIndex];
			
			parameters[parentIndex.first][parentIndex.second] = newIndex;
			parents[newIndex] = parentIndex;

			parameters[newIndex][kLeftParameterIndex] = leftIndex;
			parents[leftIndex] = Pair(newIndex, kLeftParameterIndex);
		}
		// we are probably a parameter of another element
		else
		{
			// 
		}

		if (!evaluateOrder)
		{
			return;
		}
	}


	// this is recursive descent
	// requires precondition that everything in evaluationOrder so far
	// isn't dependent on the element at the provided index
	void EvaluateOrderFrom(ElementIndex index)
	{
		for(const auto & argument : elementMapping[index].arguments)
		{
			EvaluateOrderFrom(argument.second);
		}
		evaluationOrder.push_back(index);
	}

	void EvaluateOrder()
	{
		evaluationOrder.clear();
		if (elementMapping.count() == 0)
		{
			return;
		}
		for (int i = 0; i < elementMapping.count(); i++)
		{
			if (elementMapping[i].parent.first == kNullElementIndex)
			{
				EvaluateOrderFrom(ElementIndex{i});
				break;
			}
		}
		if (evaluationOrder.count() != elementMapping.count())
		{
			// rmf todo: log error;
		}
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
	virtual Value Evaluate(
		CommandContext context,
		std::map<ParameterIndex, CRef<Value> > arguments) const override
	{
		std::vector<Value> computedValues{implementation.elementMapping.size()};
		std::map<ParameterIndex, CRef<Value> > subArguments;

		for(ElementIndex index : implementation.evaluationOrder)
		{
			const auto & elementMapping = implementation.elementMapping[index.value];
			const auto & element = elementMapping.element;
			const auto pReference = dynamic_cast<const ElementReference*>(&element);
			
			if (pReference != nullptr)
			{
				// rmf todo: post load check that this is in range
				ParameterIndex parameterIndex = pReference->parameter;

				if (arguments.contains_key(parameterIndex))
				{
					computedValues[index.value] = arguments[parameterIndex];
				}
				else
				{
					ElementParameter& parameter = parameterIndex == -1
						? element.left_parameter
						: element.right_parameters[parameterIndex];
					if (parameter.default_value != nullptr)
					{
						computedValues[index.value] = parameter.default_value;
					}
					// rmf todo: what if there isn't a default? error?
				}
				continue;
			}
			subArguments.clear();
			for(auto [parameterIndex, elementValueIndex] : elementMapping.arguments)
			{
				subArguments[parameterIndex] = cref(computedValues[elementValueIndex.value]);
			}
			computedValues[index.value] = element.Evaluate(context, subArguments);
		}

		// last computed value is return, yes? or is this a faulty assumption?
		return computedValues[computedValues.count - 1];
	}
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
