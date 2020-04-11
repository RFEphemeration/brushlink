#ifndef COMMAND_PARAMETER_HPP
#define COMMAND_PARAMETER_HPP

#include "ErrorOr.hpp"

namespace Command
{

struct CommandElement;

struct CommandParameter
{
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
		Value value = CHECK_RETURN(EvaluateRepeatable(context));
		if (!std::holds_alternative<T>(value))
		{
			return Error("Element is of unexpected type");
		}
		return std::get<T>(value);
	}

	// todo: should this take into consideration the current state of parameters?
	// should probably have a separate function for that
	virtual std::set<ElementType> GetAllowedTypes() = 0;

	virtual bool IsRequired() = 0;

	virtual bool IsSatisfied() = 0;

	virtual CommandElement * GetLastArgument() = 0;

	virtual ErrorOr<Success> SetArgumentInternal(CommandElement * argument) = 0;

	virtual ErrorOr<Value> Evaluate(CommandContext & context) = 0;

	virtual ErrorOr<Repeatable<Value>> EvaluateRepeatable(CommandContext & context)
	{
		Value value = CHECK_RETURN(Evaluate(context));
		return {value};
	}
}

struct ParamSingleRequired : CommandParameter
{
	const ElementType type;
	std::unique_ptr<CommandElement> argument;

	ParamSingleRequired(ElementType type)
		: type(type)
	{ }

	std::set<ElementType> GetAllowedTypes() override { return {type}; }

	bool IsRequired() override { return true; }

	bool IsSatisfied() override { return argument != nullptr; }

	CommandElement * GetLastArgument() override { return argument; }

	ErrorOr<Success> SetArgumentInternal(CommandElement * argument) override;

	ErrorOr<Value> Evaluate(CommandContext & context) override;
}

struct ParamSingleOptional : ParamSingleRequired
{
	ElementName default_value;

	ParamSingleOptional(ElementType type, ElementName default_value)
		: ParamSingleRequired(type)
		, default_value(default_value)
	{
		// todo: check default value is of correct type
	}

	bool IsRequired() override { return false; }

	bool IsSatisfied() override { return true; }

	ErrorOr<Value> Evaluate(CommandContext & context);
}

struct ParamRepeatableRequired : CommandParameter
{
	const ElementType type;
	Repeatable<OwnedPtr<CommandElement> > arguments;

	ParamRepeatableRequired(ElementType type)
		: type(type)
	{ }

	std::set<ElementType> GetAllowedTypes() override { return {type}; }

	bool IsRequired() override { return true; }

	bool IsSatisfied() override { return !arguments.empty(); }

	CommandElement * GetLastArgument() override;

	// todo: should this be passed in as a unique_ptr?
	ErrorOr<Success> SetArgumentInternal(CommandElement * argument) override;

	ErrorOr<Value> Evaluate(CommandContext & context) override;

	ErrorOr<Repeatable<Value> > EvaluateRepeatable(CommandContext & context) override;
}

struct ParamRepeatableOptional : ParamRepeatableRequired
{
	ElementName default_value;

	ParamRepeatableOptional(ElementType type, ElementName default_value)
		: ParamRepeatableRequired(ElementType type)
		, default_value(default_value)
	{
		// todo: assert default value is of correct type
	}

	bool IsSatisfied() override { return true; }

	ErrorOr<Value> Evaluate(CommandContext & context) override;

	ErrorOr<Repeatable<Value> > EvaluateRepeatable(CommandContext & context) override;
}

struct OneOf : CommandParameter
{
	std::vector<OwnedPtr<CommandParameter> > possibilities;
	int chosen_index;

	OneOf(std::vector<OwnedPtr<CommandParameter> > possibilities)
		: possibilities(possibilities)
		, chosen_index(-1)
	{ }

	std::set<ElementType> GetAllowedTypes() override;

	bool IsRequired() override;

	bool IsSatisfied() override;

	ErrorOr<Success> SetArgumentInternal(CommandElement * argument) override;

	CommandElement * GetLastArgument() override;

	ErrorOr<Value> Evaluate(CommandContext & context) override;

	ErrorOr<Repeatable<Value> > EvaluateRepeatable(CommandContext & context) override;
}


} // namespace Command

#endif // COMMAND_PARAMETER_HPP
