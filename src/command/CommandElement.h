
struct CommandElement;

struct CommandParameter
{
	virtual std::set<ElementType> GetAllowedTypes() = 0;

	virtual bool IsRequired() = 0;

	virtual bool IsSatisfied() = 0;

	ErrorOr<Success> SetArgument(CommandElement * argument)
	{
		if (argument == nullptr)
		{
			return Error("Argument is null");
		}
		if (!GetAllowedTypes().contains(argument->GetType()))
		{
			return Error("Argument is of unacceptable type");
		}
		return SetArgumentImplementation(argument);
	}

	virtual ErrorOr<Success> SetArgumentInternal(CommandElement * argument) = 0;

	template<typename T>
	ErrorOr<T> EvaluateAs(CommandContext & context)
	{
		Value value = CHECK_RETURN(Evaluate(context));
		if (!std::holds_alternative<T>(value))
		{
			return Error("Element is of unexpected type");
		}
		return std::get<T>(value);
	}

	template<typename T>
	ErrorOr<Repeatable<T> > EvaluateAsRepeatable(CommandContext & context)
	{
		Value value = CHECK_RETURN(EvaluateRepeatable(context));
		if (!std::holds_alternative<T>(value))
		{
			return Error("Element is of unexpected type");
		}
		return std::get<T>(value);
	}

	virtual ErrorOr<Value> Evaluate(CommandContext & context) = 0;

	virtual ErrorOr<Repeatable<Value>> EvaluateRepeatable(CommandContext & context)
	{
		Value value = CHECK_RETURN(Evaluate(context));
		return {value};
	}
}

struct SingleRequired : CommandParameter
{
	const ElementType type;
	OwnedPtr<CommandElement> argument;

	SingleRequired(ElementType type)
		: type(type)
	{ }

	std::set<ElementType> GetAllowedTypes() { return {type}; }

	bool IsRequired() { return true; }

	bool IsSatisfied() { return argument != nullptr; }

	ErrorOr<Success> SetArgumentInternal(CommandElement * argument)
	{
		if (argument != nullptr)
		{
			return Error("Cannot accept multiple arguments for this parameter");
		}
		this.argument = argument;
		return Success();
	}

	ErrorOr<Value> Evaluate(CommandContext & context)
	{
		if (argument == nullptr)
		{
			return Error("Parameter argument is missing, cannot evaluate");
		}
		return argument->Evaluate(context);
	}
}

struct SingleOptional : SingleRequired
{
	ElementName default_value;

	SingleOptional(ElementType type, ElementName default_value)
		: SingleRequired(type)
		, default_value(default_value)
	{
		// todo: check default value is of correct type
	}

	bool IsRequired() { return false; }

	bool IsSatisfied() { return true; }

	ErrorOr<Value> Evaluate(CommandContext & context)
	{
		if (argument == nullptr)
		{
			// todo: get value for default
		}
		return argument->Evaluate(context);
	}

}

struct RepeatableRequired : CommandParameter
{
	const ElementType type;
	Repeatable<OwnedPtr<CommandElement> > arguments;

	RepeatableRequired(ElementType type)
		: type(type)
	{ }

	std::set<ElementType> GetAllowedTypes() { return {type}; }

	bool IsRequired() { return true; }

	bool IsSatisfied() { return !arguments.empty(); }

	// todo: should this be passed in as a unique_ptr?
	ErrorOr<Success> SetArgumentInternal(CommandElement * argument)
	{
		this.arguments.append(argument);
		return Success();
	}

	ErrorOr<Value> Evaluate(CommandContext & context)
	{
		if (arguments.size() > 1)
		{
			return Error("There are too many arguments here for evaluating as a single");
		}
		if (arguments.size() == 0)
		{
			return Error("Parameter argument is missing, cannot evaluate");	
		}
		return arguments[0]->Evaluate(context);
	}

	ErrorOr<Repeatable<Value> > EvaluateRepeatable(CommandContext & context)
	{
		Repeatable<Value> values;

		for (auto argument : arguments)
		{
			values.append(CHECK_RETURN(argument->Evaluate(context)));
		}
		return values;
	}
}

struct RepeatableOptional : RepeatableRequired
{
	ElementName default_value;

	RepeatableOptional(ElementType type, ElementName default_value)
		: RepeatableRequired(ElementType type)
		, default_value(default_value)
	{
		// todo: assert default value is of correct type
	}

	bool IsSatisfied() { return true; }

	ErrorOr<Value> Evaluate(CommandContext & context)
	{
		if (arguments.size() > 1)
		{
			return Error("There are too many arguments here for evaluating as a single");
		}
		if (arguments.size() == 0)
		{
			return Error("Parameter argument is missing, cannot evaluate");	
		}
		return arguments[0]->Evaluate(context);
	}

	ErrorOr<Repeatable<Value> > EvaluateRepeatable(CommandContext & context)
	{
		Repeatable<Value> values;

		for (auto argument : arguments)
		{
			values.append(CHECK_RETURN(argument->Evaluate(context)));
		}
		return values;
	}
}

struct CommandElement
{
	const ElementType type;
	const std::vector<ElementType> parameter_types;

	ElementType Type()
	{
		return type;
	}

	int ParameterCount()
	{
		return parameter_types.size();
	}

	ElementType ParameterType(int index)
	{
		if (index >= ParameterCount())
		{
			return ElementType.None;
		}

		return parameter_types[index];
	}

	bool AddParameter(int index, CommandElement * parameter)
	{
		if (index >= ParameterCount())
		{
			return false;
		}
		if (parameter == nullptr)
		{
			return false;
		}
		if (parameter->Type() != ParameterType(index))
		{
			return false;
		}
		return InternalAddParameter(index, parameter);
	}

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

	virtual bool AddParameterImplementation(int index, CommandElement * parameter)
	{
		return false;
	};

	virtual bool ParametersMet()
	{
		return true;
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

	ErrorOr<Value> Evaluate(const CommandContext & context)
	{
		return value;
	}
}

struct SetCurrentSelection : CommandElement
{
	CommandElement * actors;

	SetCurrentSelection()
		: CommandElement(ElementType.Action, {ElementType.UnitGroup})
	{}

	bool AddParameterImplementation(int index, CommandElement * parameter)
	{
		if (actors != nullptr)
		{
			return false;
		}
		actors = parameter
	}

	bool ParametersMet()
	{
		return actors != nullptr && actors->ParametersMet();
	}

	ErrorOr<Value> Evaluate(const CommandContext & context)
	{
		UnitGroup actor_units = CHECK_RETURN(actors.EvaluateAs<UnitGroup>(context));
		return context.SetCurrentSelection(actor_units);
	}
}

struct Move : CommandElement
{
	CommandElement * actors;
	CommandElement * location;

	SetCurrentSelection()
		: CommandElement(ElementType.Action, {ElementType.UnitGroup, ElementType.Location})
	{}
}

struct CurrentSelection : CommandElement
{
	CurrentSelection()
		: CommandElement(ElementType.UnitGroup, {})
	{}

	ErrorOr<Value> Evaluate(const CommandContext & context)
	{
		return context.CurrentSelection();
	}
}