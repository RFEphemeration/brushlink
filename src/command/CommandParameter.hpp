#ifndef COMMAND_PARAMETER_HPP
#define COMMAND_PARAMETER_HPP

#include "ErrorOr.hpp"
#include "ElementType.h"
#include "CommandContext.h"

namespace Command
{

struct CommandElement;

struct CommandParameter
{
	CommandParameter() = default;

	CommandParameter(const CommandParameter & other) { }

	// this is intended to only be used by value_ptr internals
	virtual CommandParameter * clone() const
	{
		// @Incomplete why can't we use abstract classes with value_ptr?
		return nullptr;
	}

	virtual ~CommandParameter() = default;

	ErrorOr<Success> SetArgument(CommandElement * argument);

	template<typename T>
	ErrorOr<T> EvaluateAs(CommandContext & context)
	{
		Value value = CHECK_RETURN(Evaluate(context));
		if (!std::holds_alternative<T>(value))
		{
			return Error("Element is of unexpected type");
		}
		return std::get<T>(value);
	}

	template<typename T>
	ErrorOr<Repeatable<T> > EvaluateAsRepeatable(CommandContext & context)
	{
		Repeatable<Value> values = CHECK_RETURN(EvaluateRepeatable(context));
		Repeatable<T> ret;
		for (auto value : values)
		{
			if (!std::holds_alternative<T>(value))
			{
				return Error("Element is of unexpected type");
			}
			ret.push_back(std::get<T>(value));
		}
		return ret;
	}

	virtual std::string GetPrintString(std::string line_prefix) = 0;

	virtual bool IsRequired() = 0;

	// todo: should this take into consideration the current state of parameters?
	// should probably have a separate function for that
	virtual Set<ElementType::Enum> GetAllowedTypes() = 0;

	virtual bool IsSatisfied() = 0;

	virtual CommandElement * GetLastArgument() = 0;

	virtual ErrorOr<Success> SetArgumentInternal(CommandElement * argument) = 0;

	virtual ErrorOr<Value> Evaluate(CommandContext & context) = 0;

	virtual ErrorOr<Repeatable<Value> > EvaluateRepeatable(CommandContext & context)
	{
		Value value = CHECK_RETURN(Evaluate(context));
		return Repeatable<Value>{value};
	}
};

// @Incomplete is this even necessary? or should we instead have identity defaults
value_ptr<CommandParameter> Param(
	ElementType::Enum type,
	ElementName default_value,
	OccurrenceFlags::Enum flags = static_cast<OccurrenceFlags::Enum>(0x0));

value_ptr<CommandParameter> Param(ElementType::Enum type, ElementName default_value)
{
	return Param(type, default_value, OccurrenceFlags::Optional);
}

value_ptr<CommandParameter> Param(ElementType::Enum type, OccurrenceFlags::Enum flags = static_cast<OccurrenceFlags::Enum>(0x0))
{
	// @Incomplete optional without default value
	return Param(type, "", flags);
}

struct ParamSingleRequired : CommandParameter
{
	const ElementType::Enum type;
	value_ptr<CommandElement> argument;

	ParamSingleRequired(ElementType::Enum type)
		: type(type)
	{ }

	ParamSingleRequired(const ParamSingleRequired & other)
		: CommandParameter(other)
		, type(other.type)
		, argument(other.argument)
	{ }

	// this is intended to only be used by value_ptr internals
	virtual CommandParameter * clone() const override
	{
		return new ParamSingleRequired(*this);
	}

	std::string GetPrintString(std::string line_prefix) override;

	Set<ElementType::Enum> GetAllowedTypes() override
	{
		if (argument.get() == nullptr)
		{
			return {type};
		}
		else
		{
			return {};
		}
	}

	bool IsRequired() override { return true; }

	bool IsSatisfied() override;

	CommandElement * GetLastArgument() override { return argument.get(); }

	ErrorOr<Success> SetArgumentInternal(CommandElement * argument) override;

	ErrorOr<Value> Evaluate(CommandContext & context) override;
};

struct ParamSingleOptional : ParamSingleRequired
{
	// @Incomplete should optional without default value be possible?
	ElementName default_value;

	ParamSingleOptional(ElementType::Enum type, ElementName default_value)
		: ParamSingleRequired(type)
		, default_value(default_value)
	{
		// todo: check default value is of correct type
	}

	ParamSingleOptional(const ParamSingleOptional & other)
		: ParamSingleRequired(other)
		, default_value(other.default_value)
	{ }

	// this is intended to only be used by value_ptr internals
	virtual CommandParameter * clone() const override
	{
		return new ParamSingleOptional(*this);
	}

	std::string GetPrintString(std::string line_prefix) override;

	bool IsRequired() override { return false; }

	bool IsSatisfied() override;

	ErrorOr<Value> Evaluate(CommandContext & context) override;
};

struct ParamRepeatableRequired : CommandParameter
{
	const ElementType::Enum type;
	std::vector<value_ptr<CommandElement> > arguments;

	ParamRepeatableRequired(ElementType::Enum type)
		: type(type)
	{ }

	ParamRepeatableRequired(const ParamRepeatableRequired & other)
		: CommandParameter(other)
		, type(other.type)
		, arguments(other.arguments)
	{ }

	// this is intended to only be used by value_ptr internals
	virtual CommandParameter * clone() const override
	{
		return new ParamRepeatableRequired(*this);
	}

	std::string GetPrintString(std::string line_prefix) override;

	Set<ElementType::Enum> GetAllowedTypes() override { return {type}; }

	bool IsRequired() override { return true; }

	bool IsSatisfied() override;

	CommandElement * GetLastArgument() override;

	// todo: should this be passed in as a unique_ptr?
	ErrorOr<Success> SetArgumentInternal(CommandElement * argument) override;

	ErrorOr<Value> Evaluate(CommandContext & context) override;

	ErrorOr<Repeatable<Value> > EvaluateRepeatable(CommandContext & context) override;
};

struct ParamRepeatableOptional : ParamRepeatableRequired
{
	ElementName default_value;

	ParamRepeatableOptional(ElementType::Enum type, ElementName default_value)
		: ParamRepeatableRequired(type)
		, default_value(default_value)
	{
		// todo: assert default value is of correct type
	}

	ParamRepeatableOptional(const ParamRepeatableOptional & other)
		: ParamRepeatableRequired(other)
		, default_value(other.default_value)
	{ }

	// this is intended to only be used by value_ptr internals
	virtual CommandParameter * clone() const override
	{
		return new ParamRepeatableOptional(*this);
	}

	std::string GetPrintString(std::string line_prefix) override;

	bool IsSatisfied() override;

	ErrorOr<Value> Evaluate(CommandContext & context) override;

	ErrorOr<Repeatable<Value> > EvaluateRepeatable(CommandContext & context) override;
};

struct OneOf : CommandParameter
{
	std::vector<value_ptr<CommandParameter> > possibilities;
	int chosen_index;

	OneOf(std::vector<value_ptr<CommandParameter> > possibilities)
		: possibilities(possibilities)
		, chosen_index(-1)
	{ }

	OneOf(const OneOf & other)
		: CommandParameter(other)
		, possibilities(other.possibilities)
		, chosen_index(other.chosen_index)
	{ }

	// this is intended to only be used by value_ptr internals
	virtual CommandParameter * clone() const override
	{
		return new OneOf(*this);
	}


	std::string GetPrintString(std::string line_prefix) override;

	Set<ElementType::Enum> GetAllowedTypes() override;

	bool IsRequired() override;

	bool IsSatisfied() override;

	ErrorOr<Success> SetArgumentInternal(CommandElement * argument) override;

	CommandElement * GetLastArgument() override;

	ErrorOr<Value> Evaluate(CommandContext & context) override;

	ErrorOr<Repeatable<Value> > EvaluateRepeatable(CommandContext & context) override;
};

} // namespace Command

#endif // COMMAND_PARAMETER_HPP
