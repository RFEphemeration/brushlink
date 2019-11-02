#ifndef BRUSHLINK_ELEMENT_DEFINITIONS_HPP
#define BRUSHLINK_ELEMENT_DEFINITIONS_HPP

namespace Command
{

struct Value
{

}

struct ElementDefinition
{
	virtual ErrorOr<Value> Evaluate(
		const CommandContext & context,
		const ElementNode & node) const;

	// how to avoid circular dependency with this?
	// should definitions not be part of declarations?
	virtual bool ValidateDefinitionMeetsDeclaration(
		const ElementDeclaration & decl) const;
}

template<typename TRet, typename ... TArgs>
struct ElementDefinitionAtom : ElementDefinition
{
	ErrorOr<TRet> (*underlying_function)(const CommandContext &, TArgs...);

	virtual ErrorOr<Value> Evaluate(
		const CommandContext & context,
		const ElementNode & node) const override
	{
		CurriedFunctor starting_functor {
			FunctionPointer { underlying_function },
			context
		};

		const ElementDeclaration * decl = ElementDictionary::GetDeclaration(node.token.name);
		if (decl == nullptr)
		{
			return Error("couldn't find delaration for node that has a definition: " + node.token.name.value);
		}

		auto curried_functor = CHECK_RETURN(CurryArgs(
			context,
			node,
			starting_functor,
			decl->GetMinParameterIndex()));

		return curried_functor();
	}

	template<typename TNext, typename ... TRemaining>
	ErrorOr<CurriedFunctor<ErrorOr<TRet> > > CurryArgs(
		const CommandContext & context,
		const ElementNode & node
		CurriedFunctor<ErrorOr<TRet>, TNext, TRemaining...> so_far,
		ParameterIndex index)
	{
		CurriedFunctor next {
			so_far,
			CHECK_RETURN(node.EvaluateParameterAs<TNext>(context, index))
		};
		if constexpr (sizeof...(TRemaining) == 0)
		{
			return next;
		}
		else
		{
			index.value++;
			return CurryArgs(context, node, next, index);
		}
	}
}

struct ElementDefinitionLiteral : ElementDefinition
{
	Value value;

	ElementDefinitionLiteral(Value value)
		, value(value)
	{ }

	virtual ErrorOr<Value> Evaluate(
		const CommandContext & context,
		const ElementNode & node) const override
	{
		return value;
	}
}

struct ElementDefinitionReference : ElementDefinition
{
	ParameterIndex index;

	ElementDefinitionReference(ParameterIndex index)
		:index(index)
	{ }

	virtual ErrorOr<Value> Evaluate(
		const CommandContext & context,
		const ElementNode & node) const override
	{
		return context.GetParameterReferenceValue(index);
	}
}

struct ElementDefinitionWord : ElementDefinition
{
	std::vector<ElementName> definition_stream;
	Parser definition_tree;

	// we will probably want to collapse the definition tree into a Functor
	// in which case this will end up being a typed pointer

	// rmf todo: how to handle parameters?
	virtual ErrorOr<Value> Evaluate(
		const CommandContext & context,
		const ElementNode & node) const override
	{
		CommandContext sub_context{context};
		// rmf todo: will probably move this to constructor for CommandContext
		sub_context.parameter_reference_values = node.EvaluateParameters(context);
		return definition_tree.root.Evalutate(context);
	}
}

namespace ElementDefinitionAbbreviations
{
	using AtomDfn = ElementDefinitionAtom<TRet, TArgs...>;
	using LiteralDfn = ElementDefinitionLiteral;
	using ReferenceDfn = ElementDefinitionReference;
	using WordDfn = ElementDefinitionWord;

	template<typename TRet, typename ... TArgs>
	OwnedPtr<ElementDefinition> Atom(ErrorOr<TRet> (*underlying_function)(const CommandContext &, TArgs...))
	{
		return new ElementDefinitionAtom<TRet, TArgs...>{underlying_function};
	}

	template<typename T>
	OwnedPtr<ElementDefinition> Literal(T&& value)
	{
		return new ElementDefinitionLiteral{Value{value}};
	}

	OwnedPtr<ElementDefinition> Reference(ParameterIndex index)
	{
		return new ElementDefinitionReference{index};
	}

	OwnedPtr<ElementDefinition> Word(std::vector<ElementName>&& definition_stream)
	{
		// bool argument is to delay parsing
		return new ElementDefinitionWord{definition_stream, true};
	}
	

} // namespace ElementDefinitionAbbreviations



} // namespace Command

#endif // BRUSHLINK_ELEMENT_DEFINITIONS_HPP