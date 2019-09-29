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
		static_assert(size_of...(TArgs) >= 2);
	}

	ElementAtom(
		ErrorOr<TRet> (*underlying_function)(CommandContext, TArgs...),
		std::vector<ElementParameter> right_parameters)
		: Element(ValueTypeOf<TRet>(), right_parameters)
	{

	}

	ElementAtom(
		ErrorOr<TRet> (*underlying_function)(CommandContext, TArgs...),
		value_ptr<ElementParameter> left_parameter)
		: Element(ValueTypeOf<TRet>(), left_parameter)
	{
		static_assert(size_of...(TArgs) == 1);
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

// for operator overloading based on parameters
struct ElementOneOf : Element
{
	
}