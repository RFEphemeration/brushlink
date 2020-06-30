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
	const std::optional<ValueName> name;
	// often only allow one argument but the second indirection in that case
	// is worth it for the common interface
	std::vector<value_ptr<Element> > arguments;

	virtual ~Parameter() = default;

	// for use by value_ptr
	virtual Parameter * clone() const = 0;

	// Perimeter interface
	virtual Element * GetLastArgument();
	virtual bool IsRequired() const = 0;

	// IEvaluable interface common implementations
	virtual std::string GetPrintString(std::string line_prefix) const override;

	virtual bool IsSatisfied() const override;
	virtual void GetAllowedTypes(AllowedTypes & allowed) const override = 0;
	virtual bool IsExplicitBranch() const override;

	virtual void GetAllowedTypes(AllowedTypes & allowed) const override = 0;

	virtual ErrorOr<bool> AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count) override;

	virtual ErrorOr<Removal> RemoveLastExplicitElement() override;

	virtual ErrorOr<Variant> Evaluate(Context & context) const override;

	virtual ErrorOr<std::vector<Variant> > EvaluateRepeatable(Context & context) const override;
};

template<bool repeatable, bool optional>
struct Parameter_Basic : public Parameter
{
	// only used if optional is true. Todo: move this elsewhere?
	const std::optional<ElementName> default_value;

	const Variant_Type type;

	// for use by value_ptr
	Parameter * clone() const override
	{
		return new Parameter(*this);
	}

	// Parameter interface
	bool IsRequired() const override { return !optional; }

	// IEvaluable interface
	std::string GetPrintString(std::string line_prefix) const override;

	bool IsSatisfied() const override;
	
	void GetAllowedTypes(AllowedTypes & allowed) const override;

	ErrorOr<bool> AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count) override;
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
