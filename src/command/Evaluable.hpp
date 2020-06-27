#ifndef BRUSHLINK_EVALUABLE_HPP
#define BRUSHLINK_EVALUABLE_HPP

#include "ErrorOr.hpp"

#include "Variant.h"

namespace Command
{

struct Element : public Evaluable;
struct Parameter : public Evaluable;

struct Evaluable
{
	virtual bool RequirementsSatisfied() = 0;
	virtual bool IsExplicit() = 0;
	virtual ErrorOr<bool> AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count) = 0;
	virtual void GetAllowedTypes(AllowedTypes & allowed) = 0;
	virtual std::string GetPrintString(std::string line_prefix) = 0;
	virtual ErrorOr<Value> Evaluate(Context & context) = 0;

	template<typename T>
	ErrorOr<T> EvaluateAs(Context & context)
	{
		// Value is used for OneOf
		if constexpr(std::is_same<T, Value>::value)
		{
			return Evaluate(context);
		}
		else
		{
			Value value = CHECK_RETURN(Evaluate(context));
			if (!std::holds_alternative<T>(value))
			{
				return Error("Element is of unexpected type");
			}
			return std::get<T>(value);
		}
		{
			Value value = CHECK_RETURN(Evaluate(context));
			if (!std::holds_alternative<T>(value))
			{
				/*
				if constexpr(std::is_same<T,Location>::value)
				{
					if (std::holds_alternative<Point>(value))
					{
						return Location{std::get<Point>(value)};
					}
					else if (std::holds_alternative<Line>(value))
					{
						return Location{std::get<Line>(value)};
					}
					else if (std::holds_alternative<Direction>(value))
					{
						return Location{std::get<Direction>(value)};
					}
					else if (std::holds_alternative<Area>(value))
					{
						return Location{std::get<Area>(value)};
					}
				}
				*/
				return Error("Element is of unexpected type");
			}
			return std::get<T>(value);
		}
	}


	AllParametersSatisfied vs IsSatisfied
		Append vs Set
		IsExplicitOrHasExplicitChild vs HasExplicitArgOrChild
		GetPrintString
		Evaluate
		EvaluateAs
		GetAllowedArgumentTypes vs GetAllowedTypes

}

} // namespace Command

#endif // BRUSHLINK_EVALUABLE_HPP
