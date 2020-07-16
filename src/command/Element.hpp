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

protected:
	std::queue<Parameter *> GetParams();
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

	ErrorOr<Variant> Evaluate(Context & context) const override
	{
		return Variant{value};
	}
};

struct GetNamedValue : public Element
{
	GetNamedValue();

	GetNamedValue(ValueName name);

	Element * clone() const override
	{
		return new GetNamedValue(*this);
	}

	ErrorOr<Variant> Evaluate(Context & context) const override;

	// typical element evaluation paths should only ever return one value
	// this should only be used inside of Parameter::EvaluateRepeatable
	// after an explicit dynamic cast
	ErrorOr<std::vector<Variant>> EvaluateRepeatable(Context & context) const override;
}

struct ElementFunction : public Element
{
	value_ptr<Element> implementation;

	virtual Element * clone() const override
	{
		return new ElementWord(*this);
	}

	ErrorOr<Variant> Evaluate(Context & context) const override;
};

template<typename TRet, typename ... TArgs>
struct BuiltinFunction : public Element
{
	std::variant<
		ErrorOr<TRet>(Context::*)(TArgs...),
		ErrorOr<TRet>(*)(TArgs...)> eval_func;
	std::string (*print_func)(const Element & element, std::string) ;

	virtual Element * clone() const override
	{
		return new BuiltinFunction(*this);
	}

	std::string GetPrintString(std::string line_prefix) const override
	{
		if (print_func != nullptr)
		{
			return (*print_func)(*this, line_prefix);
		}
		return Element::GetPrintString(line_prefix);
	}

	template<typename TNext, typename ... TRest>
	ErrorOr<std::tuple<TNext, TRest...>> MakeEvaluatedArgs(
		std::queue<Parameter *> & params, Context & context)
	{
		if (params.empty() && !std::is_same<TNext, Context &>::value)
		{
			return Error("Not enough parameters during evaluation");
		}
		auto next = [&] -> ErrorOr<TNext>
		{
			if constexpr(std::is_same<TNext, Context &>::value)
				return context;
			else if constexpr (std::is_same<TNext, const Parameter *>::value)
				// no need to copy if parameter is const
				return params.pop.get();
			else if constexpr (IsSpecialization<TNext, std::vector>::value)
				return params.pop->template EvaluateAsRepeatable<TNext::value_type>(context);
			else
				return params.pop()->template EvaluateAs<T>(context);
		}();
		if constexpr (sizeof...(TRest) == 0)
		{
			if (!params.empty())
			{
				return Error("Too many parameters during evaluation");
			}
			return std::tuple<T>{ CHECK_RETURN(next) };
		}
		else
		{
			// evaluate the rest even if the first was an error
			// is this necessary? do we ever recover from errors?
			// aren't there more likely to be cascading errors then?
			auto rest = MakeEvaluatedArgs<TRest...>(params, context);
			return std::tuple_cat(
				std::tuple<T>{ CHECK_RETURN(next) },
				CHECK_RETURN(rest)
			);
		}
	}

	ErrorOr<Variant> Evaluate(Context & context) const override
	{
		if constexpr(sizeof...(TArgs) == 0)
		{
			return Variant{CHECK_RETURN((context.*func)())};
		}
		else
		{
			std::queue<Parameter *> params = GetParams();
			if (eval_func.index == 0)
			{
				auto args = CHECK_RETURN(MakeEvaluatedArgs<Context &, TArgs...>(params, context));
				return Variant{
					CHECK_RETURN(std::apply(*std::get<0>(eval_func), args))
				};
			}
			else
			{
				auto args = CHECK_RETURN(MakeEvaluatedArgs<TArgs...>(params, context));
				return Variant{
					CHECK_RETURN(std::apply(*std::get<1>(eval_func), args))
				};
			}
		}
	}
};

} // namespace Command

#endif // BRUSHLINK_ELEMENT_HPP
