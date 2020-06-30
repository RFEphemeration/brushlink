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
struct Literal : Element
{
	TVal value;

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
	std::priority_queue<Parameter *> & params, Context & context)
{
	if (params.empty())
	{
		return Error("Not enough parameters during evaluation");
	}
	ErrorOr<T> next = [&]
	{
		if constexpr (std::is_same<T, value_ptr<Element> >::value)
		{
			return value_ptr<Element>{clone()};
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
		return std::tuple<T>{
			CHECK_RETURN(next.GetValue())
		};
	}
	else
	{
		// evaluate the rest even if the first was an error
		auto rest = EvaluateParameters<TArgs...>(params, context);
		return std::tuple_cat(
			std::tuple<T>{
				CHECK_RETURN(result.GetValue())
			},
			CHECK_RETURN(rest.GetValue())
		);
	}
}

template<typename TRet, typename ... TArgs>
struct ContextFunction : Element
{
	ErrorOr<TRet> (Context::*func)(TArgs...);
	std::string (*print_func)(const ContextFunction & element, std::string);

	virtual Element * clone() const override
	{
		return new ContextFunction(*this);
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
			return Value{CHECK_RETURN((context.*func)())};
		}
		else
		{
			std::priority_queue<Parameter *> params;
			if (left_parameter)
			{
				params.push(left_parameter.get())
			}
			for (auto & param : parameters)
			{
				params.push(param.get());
			}
			auto args = CHECK_RETURN(EvaluateParameters<TArgs...>(params, context));
			auto context_and_args = std::tuple_cat(std::tuple<Context &>{context}, args);
			return Variant{
				CHECK_RETURN(std::apply(*func, context_and_args))
			};
		}
	}
};

template<typename TRet, typename ... TArgs>
struct GlobalFunction : Element
{
	ErrorOr<TRet> (*func)(TArgs...);
	std::string (*print_func)(const GlobalFunction & element, std::string);

	virtual Element * clone() const override
	{
		return new GlobalFunction(*this);
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
			return Value{CHECK_RETURN((*func)())};
		}
		else
		{
			std::priority_queue<Parameter *> params;
			if (left_parameter)
			{
				params.push(left_parameter.get())
			}
			for (auto & param : parameters)
			{
				params.push(param.get());
			}
			auto args = CHECK_RETURN(EvaluateParameters<TArgs...>(params, context));
			return Variant{
				CHECK_RETURN(std::apply(*func, args))
			};
		}
	}
};

} // namespace Command

#endif // BRUSHLINK_ELEMENT_HPP
