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

	virtual std::string GetPrintString(std::string line_prefix) const override;
	virtual ErrorOr<Variant> Evaluate(Context & context) const override;

	bool IsSatisfied() const override;
	bool IsExplicitBranch() const override;
	void GetAllowedTypes(AllowedTypes & allowed) const override;

	ErrorOr<bool> AppendArgument(Context & context, value_ptr<Element>&& next, int &skip_count) override;

	ErrorOr<Removal> RemoveLastExplicitElement() override;
};

} // namespace Command

#endif // BRUSHLINK_ELEMENT_HPP
