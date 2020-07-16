#ifndef BRUSHLINK_ELEMENT_HPP
#define BRUSHLINK_ELEMENT_HPP

#include "IEvaluable.hpp"

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

struct Element : public IEvaluable
{
	const ElementName name;
	// @Feature MultipleReturnValues
	// @Feature varying return value based on parameter
	const Variant_Type type;
	const value_ptr<Parameter> left_parameter;
	const std::vector<value_ptr<Parameter> > parameters;

	// these changes depending on contextual use, so they cannot be const
	Implicit implicit;
	value_ptr<Element> * location_in_parent;

	virtual ~Element() = default;

	virtual Element * clone() const
	{
		return new Element(*this);
	}

	virtual std::string GetPrintString(std::string line_prefix) const override;
	virtual ErrorOr<Variant> Evaluate(Context & context) const override;

	bool IsSatisfied() const override;
	bool IsExplicitBranch() const override;
	void GetAllowedTypes(AllowedTypes & allowed) const override;

	ErrorOr<bool> AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count) override;

	ErrorOr<Removal> RemoveLastExplicitElement() override;
};

template<typename TVal>
struct Literal : public Element
{
	TVal value;

	Literal(TVal value, ElementName name = {""}) // most literals don't have names
		: Element{
			name,
			GetVariantType<TVal>(),
			{}, // no left parameter
			{}, // no parameters
		}
		, value{value}
	{ }

	Element * clone() const override
	{
		return new Literal(*this);
	}

	std::string GetPrintString(std::string line_prefix) const
	{
		return ToString(TVal);
	}

	ErrorOr<Variant> Evaluate(context & context) const override
	{
		return Variant{value};
	}
};

template<typename T, typename ... TArgs>
ErrorOr<std::tuple<T, TArgs...>> EvaluateParameters(
	std::queue<Parameter *> & params, Context & context)
{
	if (params.empty())
	{
		return Error("Not enough parameters during evaluation");
	}
	ErrorOr<T> next = [&]
	{
		if constexpr (std::is_same<T, const Element *>::value)
		{
			// no need to copy if element is const
			return params.pop.get();
		}
		if constexpr (std::is_same<T, value_ptr<Element> >::value)
		{
			// do we need to clone here? or can we just return the element?
			// it would probably need to be const
			return value_ptr<Element>{params.pop->clone()};
		}
		else if constexpr (IsSpecialization<T, std::vector>::value)
		{
			return params.pop->template EvaluateAsRepeatable<T::value_type>(context);
		}
		else
		{
			return params.pop()->template EvaluateAs<T>(context);
		}
	}();
	
	if constexpr (sizeof...(TArgs) == 0)
	{
		if (!params.empty())
		{
			return Error("Too many parameters during evaluation");
		}
		return std::tuple<T>{ CHECK_RETURN(next.GetValue()) };
	}
	else
	{
		// evaluate the rest even if the first was an error
		auto rest = EvaluateParameters<TArgs...>(params, context);
		return std::tuple_cat(
			std::tuple<T>{ CHECK_RETURN(result.GetValue())},
			CHECK_RETURN(rest.GetValue())
		);
	}
}

template<typename TRet, typename TVal>
ErrorOr<TRet> ConvertForEvaluation(TVal&& value)
{
	if constexpr(!std::is_same<TRet, std::vector<Variant>>::value
		&& !std::is_same<TRet, Variant>::value)
	{
		assert(false);
	}
	if constexpr (std::is_same<TRet, TVal>::value)
	{
		return value;
	}
	else if constexpr(is_same<TRet, std::vector<Variant>>::value)
	{
		if constexpr(IsSpecialization<TVal, std::vector>::value)
		{
			// we know tval isn't vector<Variant> because that would be caught by is_same
			std::vector<Variant> ret;
			for (auto && val : value)
			{
				ret.emplace_back(std::move(val));
			}
			return ret;
		}
		else
		{
			return std::vector<Variant>{Variant{value}};
		}
	}
	else
	{
		if constexpr(IsSpecialization<TVal, std::vector>::value)
		{
			if (value.size() != 1)
			{
				return Error("Tried to evaluate repeated Element as single");
			}
			return Variant{value[0]};
		}
		else
		{
			return Variant{value};
		}
	}
}

std::queue<Parameter *> ConcatParams(
	value_ptr<Parameter> & left_parameter,
	std::vector<value_ptr<Parameter>> & parameters);

using PrintFunction = std::string (*)(const Element & element, std::string);

template<typename TRet, typename ... TArgs>
struct ContextFunction : public Element
{
	ErrorOr<TRet> (Context::*func)(TArgs...);
	PrintFunction print_func;

	virtual Element * clone() const override
	{
		return new ContextFunction(*this);
	}

	bool IsRepeatable() const override
	{
		return IsSpecialization<TRet, std::vector>::value;
	}

	std::string GetPrintString(std::string line_prefix) const override
	{
		if (print_func != nullptr)
		{
			return (*print_func)(*this, line_prefix);
		}
		return Element::GetPrintString(line_prefix);
	}

	ErrorOr<Variant> Evaluate(Context & context) const override
	{
		if constexpr(sizeof...(TArgs) == 0)
		{
			return ConvertForEvaluation<Variant, TRet>(CHECK_RETURN((context.*func)()));
		}
		else
		{
			std::queue<Parameter *> params = ConcatParams(left_parameter, parameters);
			auto args = CHECK_RETURN(EvaluateParameters<Context &, TArgs...>(params, context));
			auto context_and_args = std::tuple_cat(std::tuple<Context &>{context}, args);
			return ConvertForEvaluation<Variant, TRet>(CHECK_RETURN(std::apply(*func, context_and_args)));
		}
	}

	ErrorOr<std::vector<Variant>> EvaluateRepeatable(Context & context) const override
	{

		if constexpr(sizeof...(TArgs) == 0)
		{
			return ConvertForEvaluation<std::vector<Variant>, TRet>(CHECK_RETURN((context.*func)()));
		}
		else
		{
			std::queue<Parameter *> params = ConcatParams(left_parameter, parameters);
			auto args = CHECK_RETURN(EvaluateParameters<TArgs...>(params, context));
			auto context_and_args = std::tuple_cat(std::tuple<Context &>{context}, args);
			return ConvertForEvaluation<std::vector<Variant>, TRet>(CHECK_RETURN(std::apply(*func, context_and_args)));
		}
	}
};

// could consider player and world functions also
// using context::GetPlayer and context::GetWorld
// in order to reduce different contexts overriding each of the functions

template<typename TRet, typename ... TArgs>
struct GlobalFunction : public Element
{
	ErrorOr<TRet> (*func)(TArgs...);
	PrintFunction print_func;

	virtual Element * clone() const override
	{
		return new GlobalFunction(*this);
	}

	bool IsRepeatable() const override
	{
		return IsSpecialization<TRet, std::vector>::value;
	}

	std::string GetPrintString(std::string line_prefix) const override
	{
		if (print_func != nullptr)
		{
			return (*print_func)(*this, line_prefix);
		}
		return Element::GetPrintString(line_prefix);
	}

	ErrorOr<Variant> Evaluate(Context & context) const override
	{
		if constexpr(sizeof...(TArgs) == 0)
		{
			return ConvertForEvaluation<Variant, TRet>(CHECK_RETURN((*func)()));
		}
		else
		{
			std::queue<Parameter *> params = ConcatParams(left_parameter, parameters);
			Context child_context = context.MakeChild(Scoped{false});
			auto args = CHECK_RETURN(EvaluateParameters<TArgs...>(params, child_context));
			return ConvertForEvaluation<Variant, TRet>(CHECK_RETURN(std::apply(*func, args)));
		}
	}

	ErrorOr<std::vector<Variant>> EvaluateRepeatable(Context & context) const override
	{
		if constexpr(sizeof...(TArgs) == 0)
		{
			return ConvertForEvaluation<std::vector<Variant>, TRet>(CHECK_RETURN((*func)()));
		}
		else
		{
			std::queue<Parameter *> params = ConcatParams(left_parameter, parameters);
			Context child_context = context.MakeChild(Scoped{false});
			auto args = CHECK_RETURN(EvaluateParameters<TArgs...>(params, child_context));
			return ConvertForEvaluation<std::vector<Variant>, TRet>(CHECK_RETURN(std::apply(*func, args)));
		}
	}
};

struct ElementFunction : public Element
{
	value_ptr<Element> implementation;

	const bool repeatable;

	virtual Element * clone() const override
	{
		return new ElementWord(*this);
	}

	bool IsRepeatable() const override
	{
		return repeatable;
	}

	ErrorOr<Variant> Evaluate(Context & context) const override;

	ErrorOr<std::vector<Variant>> EvaluateRepeatable(Context & context) const override;
};

} // namespace Command

#endif // BRUSHLINK_ELEMENT_HPP
