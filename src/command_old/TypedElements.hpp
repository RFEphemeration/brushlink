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

	std::set<ValueType> ComputeValidAppendTypes() const;

	ElementIndex FindAppropriateLeftArgument(const Element & e) const;

	std::Pair<ElementIndex, ParameterIndex> FindAppropriateParentForNew(const Element & e) const;

	static ErrorOr<FragmentMapping> CreateMapping(const std::vector<CRef<Element> > &elements)
	{
		return FragmentMapping{elements};
	}
}

bool ElementMapping::ParametersMet() const
{
	if (element.left_parameter != nullptr
		&& !element.left_parameter.Optional()
		&& !arguments.contains_key(kLeftParameterIndex))
	{
		return false;
	}

	for (ParameterIndex p{0}; p.value < element.right_parameters.size(); p.value++)
	{
		if (!element.right_parameters[p.value].Optional()
			&& !arguments.contains_key(p))
		{
			return false;
		}
	}

	return true;
}

FragmentMapping::FragmentMapping(const std::vector<CRef<Element> > & elements)
{
	for(const Element & e : elements)
	{
		Append(e, false);
	}

	EvaluateOrder();
}

ErrorOr<Success>() FragmentMapping::Append(const Element & e, bool evaluateOrder = true)
{
	ElementIndex eIndex { elementMapping.count() };
	elementMapping.push_back(ElementMapping{e});

	if (e.left_parameter != nullptr)
	{
		auto leftIndex = FindAppropriateLeftParameter(e);
		if (!e.left_parameter.Optional()
			&& leftIndex == kNullElementIndex)
		{
			return Error("Left Parameter is required but could not be found");
		}
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

ElementIndex FragmentMapping::FindAppropriateLeftParameter(const Element & next) const
{
	if (next.left_parameter == nullptr
		|| elementMapping.empty())
	{
		return kNullElementIndex;
	}

	const ValueType desiredType = next.left_parameter->type;
	ElementIndex current{elementMapping.count() - 1};
	while (elementMapping[current.value].element.type == )
	while (current.value > kNullElementIndex)
	{
		if (!elementMapping[current.value].ParametersMet())
		{
			return kNullElementIndex;
		}
		else if (elementMapping[current.value].element.type == desiredType)
		{
			// rmf todo: implement left param skipping based on count of skip elements
			// which ones count? only ones typed last, right?
			// so maybe our current index initialization is wrong
			break;
		}
		else
		{
			current = elementMapping[current.value].parent.first;
		}
	}

	return current;
}



// this is recursive descent
// requires precondition that everything in evaluationOrder so far
// isn't dependent on the element at the provided index
void FragmentMapping::EvaluateOrderFrom(ElementIndex index)
{
	for(const auto & argument : elementMapping[index].arguments)
	{
		EvaluateOrderFrom(argument.second);
	}
	evaluationOrder.push_back(index);
}

ErrorOr<Success> FragmentMapping::EvaluateOrder()
{
	evaluationOrder.clear();

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
		return Error("FragmentMapping::EvaluteOrder did not map correctly");
	}

	return Success();
}

// we can't do recursive descent here because ElementReferences
// must be resolved in this scope, where they can access this Word's arguments
// even if they are arguments to other Elements in the implementation
ErrorOr<Value> ElementWord::Evaluate(
	CommandContext context,
	std::map<ParameterIndex, Value> arguments) const
{
	CHECK_RETURN(FillDefaultArguments(context, arguments));

	std::vector<Value> computedValues{implementation.elementMapping.size()};
	std::map<ParameterIndex, Value> subArguments;

	for(ElementIndex index : implementation.evaluationOrder)
	{
		const auto & elementMapping = implementation.elementMapping[index.value];
		const auto & element = elementMapping.element;
		const auto pReference = dynamic_cast<const ElementReference*>(&element);
		
		if (pReference != nullptr)
		{
			// this should be in range because it's been checked in PostLoad
			computedValues[index.value] = arguments[pReference->parameter];
			continue;
		}
		subArguments.clear();
		for(auto [parameterIndex, elementValueIndex] : elementMapping.arguments)
		{
			subArguments[parameterIndex] = computedValues[elementValueIndex.value];
		}
		computedValues[index.value] = element.Evaluate(context, subArguments);
	}

	// last computed value is return, yes? or is this a faulty assumption?
	return computedValues[computedValues.count - 1];
}

ErrorOr<Success> ElementWord::PostLoad()
{
	for(ElementIndex index : implementation.evaluationOrder)
	{
		const auto & element  = implementation.elementMapping[index.value].element;
		const auto pReference = dynamic_cast<const ElementReference*>(&element);
		
		if (pReference == nullptr)
		{
			continue;
		}

		// this reference must be in range of the parameters for this element
		if (pReference->parameter == kLeftParameterIndex
			&& left_parameter == nullptr)
		{
			return Error("ElementWord contains a reference to a left parameter when one doesn't exist");
		}
		else if (pReference->parameter != kLeftParameterIndex
			&& !right_parameters.size() > pReference->parameter.value)
		{
			return Error("ElementWord contains a reference to a right parameter that doesn't exist");
		}
	}

	return Success();
}