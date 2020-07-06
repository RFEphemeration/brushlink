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

	virtual ~Parameter() = default;

	// for use by value_ptr
	virtual Parameter * clone() const = 0;

	// Perimeter interface
	virtual Element * GetLastArgument() = 0;
	// virtual bool IsRequired() const = 0;
	virtual Set<Variant_Type> Types() const = 0;

	// IEvaluable interface common implementations
	virtual std::string GetPrintString(std::string line_prefix) const override = 0;

	virtual bool IsSatisfied() const override = 0;
	virtual void GetAllowedTypes(AllowedTypes & allowed) const override = 0;
	virtual bool IsExplicitBranch() const override = 0;

	virtual ErrorOr<bool> AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count) override = 0;

	virtual ErrorOr<Removal> RemoveLastExplicitElement() override = 0;

	virtual ErrorOr<Variant> Evaluate(Context & context) const override = 0;

	virtual ErrorOr<std::vector<Variant> > EvaluateRepeatable(Context & context) const override = 0;
};

template<bool repeatable, bool optional>
struct Parameter_Basic : public Parameter
{
	const Variant_Type type;
	// only used if optional is true. Todo: move this elsewhere?
	const std::optional<ElementName> default_value;

	// often only allow one argument but the second indirection in that case
	// is worth it for the common interface
	std::vector<value_ptr<Element> > arguments;

	// for use by value_ptr
	Parameter * clone() const override
	{
		return new Parameter(*this);
	}

	// Parameter interface
	Element * GetLastArgument() override;
	// bool IsRequired() const override { return !optional; }

	Set<Variant_Type> Types() const override { return {type}; }

	// IEvaluable interface
	std::string GetPrintString(std::string line_prefix) const override;

	bool IsSatisfied() const override;
	
	void GetAllowedTypes(AllowedTypes & allowed) const override;

	bool IsExplicitBranch() const override;

	ErrorOr<bool> AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count) override;

	ErrorOr<Removal> RemoveLastExplicitElement() override;

	ErrorOr<Variant> Evaluate(Context & context) const override;

	ErrorOr<std::vector<Variant> > EvaluateRepeatable(Context & context) const override;
};

using Parameter_SingleRequired = Parameter_Basic<false, false>;
using Parameter_RepeatableRequired = Parameter_Basic<true, false>;
using Parameter_SingleOptional = Parameter_Basic<false, true>;
using Parameter_RepeatableOptional = Parameter_Basic<true, true>;


struct Parameter_OneOf : public Parameter
{
	std::vector<value_ptr<Parameter>> options;
	std::optional<int> chosen_index;

	// for use by value_ptr
	Parameter * clone() const override
	{
		return new Parameter_OneOf(*this);
	}

	// Parameter interface
	Set<Variant_Type> Types() const override;

	// IEvaluable interface
	std::string GetPrintString(std::string line_prefix) const override;

	bool IsSatisfied() const override;

	void GetAllowedTypes(AllowedTypes & allowed) const override;

	ErrorOr<bool> AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count) override;

	ErrorOr<Variant> Evaluate(Context & context) const override;

	ErrorOr<std::vector<Variant> > EvaluateRepeatable(Context & context) const override;
};

struct Parameter_ImpliedOptions : public Parameter
{
	std::vector<value_ptr<CommandElement>> implied_options;
	std::optional<int> chosen_index;

	// for use by value_ptr
	Parameter * clone() const override
	{
		return new Parameter_ImpliedOptions(*this);
	}

	// Parameter interface
	Set<Variant_Type> Types() const override;

	// IEvaluable interface
	std::string GetPrintString(std::string line_prefix) const override;

	void GetAllowedTypes(AllowedTypes & allowed) const override;

	ErrorOr<bool> AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count) override;

	ErrorOr<Variant> Evaluate(Context & context) const override;

	ErrorOr<std::vector<Variant> > EvaluateRepeatable(Context & context) const override;
};


} // namespace Command

#endif // BRUSHLINK_PARAMETER_HPP
