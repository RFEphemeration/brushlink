#ifndef BRUSHLINK_IEVALUABLE_HPP
#define BRUSHLINK_IEVALUABLE_HPP

#include "ErrorOr.hpp"

#include "Allowed_Types.hpp"
#include "Variant.h"

namespace Command
{

struct Context;
struct Element;
struct Parameter;

enum class Removal
{
	None,
	Finished,
	ContinueRemovingImplicit,
};

struct IEvaluable
{
	virtual ~IEvaluable() = default;

	virtual std::string GetPrintString(std::string line_prefix) const = 0;

	virtual bool IsSatisfied() const = 0;
	virtual bool IsExplicitBranch() const = 0;
	virtual void GetAllowedTypes(AllowedTypes & allowed) const = 0;

	virtual ErrorOr<bool> AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count) = 0;

	// return value is whether to remove this element, also
	virtual ErrorOr<Removal> RemoveLastExplicitElement() = 0;

	virtual ErrorOr<Variant> Evaluate(Context & context) const = 0;

	virtual ErrorOr<std::vector<Variant> > EvaluateRepeatable(Context & context) const
	{
		return std::vector<Variant>{CHECK_RETURN(Evaluate(context))};
	}

	template<typename T>
	ErrorOr<T> EvaluateAs(Context & context) const
	{
		if constexpr(std::is_same<T, Variant>::value)
		{
			return Evaluate(context);
		}
		else
		{
			Variant value = CHECK_RETURN(Evaluate(context));
			if (!std::holds_alternative<T>(value))
				return Error("Type mismatch during evaluation");
			return std::get<T>(value);
		}
	}

	template<typename T>
	ErrorOr<std::vector<T> > EvaluateAsRepeatable(Context & context) const
	{
		std::vector<Variant> values = CHECK_RETURN(EvaluateRepeatable(context));
		if constexpr(std::is_same<T, Variant>::value)
		{
			return values;
		}
		else
		{
			std::vector<T> ret;
			for (auto& value : values)
			{
				if (!std::holds_alternative<T>(value))
				{
					return Error("Type mismatch during evaluation");
				}
				ret.push_back(std::get<T>(value));
			}
			return ret;
		}
	}
};

} // namespace Command

#endif // BRUSHLINK_IEVALUABLE_HPP
