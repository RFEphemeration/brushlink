#ifndef BRUSHLINK_PARAMETER_HPP
#define BRUSHLINK_PARAMETER_HPP

#include "IEvaluable.hpp"

namespace Command
{

enum class Parameter_Flags
{
	Required,
	Optional,
	Single,
	Repeatable,
};

struct Parameter : public IEvaluable
{
	virtual Element * GetLastArgument() = 0;
};

template<bool repeatable, bool optional>
struct Parameter_Basic : public Parameter
{
	// only used if optional is true. Todo: move this elsewhere?
	std::optional<ElementName> default_value;

	const Variant_Type type;
	std::vector<value_ptr<Element> > arguments;

	std::string GetPrintString(std::string line_prefix) const override;

	bool IsSatisfied() const override;
	bool IsExplicitBranch() const override;
	void GetAllowedTypes(AllowedTypes & allowed) const override;

	ErrorOr<bool> AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count) override;
	Element * GetLastArgument() override;
	// return value is whether to remove this element, also
	ErrorOr<Removal> RemoveLastExplicitElement() override;

	ErrorOr<Variant> Evaluate(Context & context) const override;

	ErrorOr<std::vector<Variant> > EvaluateRepeatable(Context & context) const override;
};

using Parameter_SingleRequired = Parameter_Basic<false, false>;
using Parameter_RepeatableRequired = Parameter_Basic<true, false>;
using Parameter_SingleOptional = Parameter_Basic<false, true>;
using Parameter_RepeatableOptional = Parameter_Basic<true, true>;


// @Feature OneOf, ImpliedOptions


struct Parameter_OneOf : public Parameter
{
	std::vector<value_ptr<Parameter>> options;
	int chosen_index{-1};
};

struct Parameter_ImpliedOptions : public Parameter
{
	std::vector<value_ptr<CommandElement>> implied_options;
};


} // namespace Command

#endif // BRUSHLINK_PARAMETER_HPP
