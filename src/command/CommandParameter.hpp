#ifndef COMMAND_PARAMETER_HPP
#define COMMAND_PARAMETER_HPP

#include "ErrorOr.hpp"

namespace Command
{

struct CommandElement;
struct CommandParameter;


// @Incomplete is this even necessary? or should we instead have identity defaults
std::unique_ptr<CommandParameter> Param(
	ElementType type,
	ElementName default_value,
	OccurrenceFlags flags = 0x0);

std::unique_ptr<CommandParameter> Param(ElementType type, ElementName default_value)
{
	return Param(type, default_value, OccurrenceFlags.Optional);
}

std::unique_ptr<CommandParameter> Param(ElementType type, OccurrenceFlags flags = 0x0)
{
	// @Incomplete optional without default value
	return Param(type, "", flags);
}

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

	virtual std::string CommandElement::GetPrintString(std::string line_prefix) = 0;

	virtual std::unique_ptr<CommandParameter> DeepCopy() = 0;

	virtual bool IsRequired() = 0;

	// todo: should this take into consideration the current state of parameters?
	// should probably have a separate function for that
	virtual std::set<ElementType> GetAllowedTypes() = 0;

	virtual bool IsSatisfied() = 0;

	virtual CommandElement * GetLastArgument() = 0;

	virtual ErrorOr<Success> SetArgumentInternal(CommandElement * argument) = 0;

	virtual ErrorOr<Value> Evaluate(CommandContext & context) = 0;

	virtual ErrorOr<Repeatable<Value> > EvaluateRepeatable(CommandContext & context)
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

	std::string CommandElement::GetPrintString(std::string line_prefix) override
	{
		if (argument != nullptr)
		{
			return argument->GetPrintString(line_prefix);
		}
		else
		{
			return "";
		}
	}

	std::unique_ptr<CommandParameter> DeepCopy() override
	{
		auto * copy = new ParamSingleRequired(type);
		if (argument != nullptr)
		{
			copy.argument = argument->DeepCopy();
		}
		return copy;
	}

	std::set<ElementType> GetAllowedTypes() override
	{
		if (argument == nullptr)
		{
			return {type};
		}
		else
		{
			return {};
		}
	}

	bool IsRequired() override { return true; }

	bool IsSatisfied() override
	{
		return argument != nullptr
		&& argument->ParametersSatisfied();
	}

	CommandElement * GetLastArgument() override { return argument; }

	ErrorOr<Success> SetArgumentInternal(CommandElement * argument) override;

	ErrorOr<Value> Evaluate(CommandContext & context) override;
}

struct ParamSingleOptional : ParamSingleRequired
{
	// @Incomplete should optional without default value be possible?
	ElementName default_value;

	ParamSingleOptional(ElementType type, ElementName default_value)
		: ParamSingleRequired(type)
		, default_value(default_value)
	{
		// todo: check default value is of correct type
	}

	std::string CommandElement::GetPrintString(std::string line_prefix) override
	{
		if (argument != nullptr)
		{
			return argument->GetPrintString(line_prefix);
		}
		else if (!default_value.IsEmpty())
		{
			return line_prefix + "(" + default_value + ")\n";
		}
		else
		{
			return "";
		}
	}

	std::unique_ptr<CommandParameter> DeepCopy() override
	{
		auto * copy = new ParamSingleOptional(type, default_value);
		if (argument != nullptr)
		{
			copy.argument = argument->DeepCopy();
		}
		return copy;
	}

	bool IsRequired() override { return false; }

	bool IsSatisfied() override { return argument == nullptr || argument->ParametersSatisfied(); }

	ErrorOr<Value> Evaluate(CommandContext & context);
}

struct ParamRepeatableRequired : CommandParameter
{
	const ElementType type;
	std::vector<std::unique_ptr<CommandElement> > arguments;

	ParamRepeatableRequired(ElementType type)
		: type(type)
	{ }

	std::string CommandElement::GetPrintString(std::string line_prefix) override
	{
		std::string print_string = "";
		for (auto arg : arguments)
		{
			print_string += art->GetPrintString(line_prefix);
		}
		return print_string;
	}

	std::unique_ptr<CommandParameter> DeepCopy() override
	{
		auto * copy = new ParamRepeatableRequired(type);
		for (auto arg : arguments)
		{
			copy->arguments.append(arg->DeepCopy());
		}
		return copy;
	}

	std::set<ElementType> GetAllowedTypes() override { return {type}; }

	bool IsRequired() override { return true; }

	bool IsSatisfied() override
	{
		return !arguments.empty()
			&& arguments[arguments.size()-1]->ParametersSatisfied();
	}

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

	std::string CommandElement::GetPrintString(std::string line_prefix) override
	{
		std::string print_string = "";
		for (auto arg : arguments)
		{
			print_string += art->GetPrintString(line_prefix);
		}
		return print_string;
		
		if (arguments.empty() && !default_value.IsEmpty())
		{
			print_string += line_prefix + "(" + default_value + ")\n";
		}

		return print_string;
	}

	std::unique_ptr<CommandParameter> DeepCopy() override
	{
		auto * copy = new ParamRepeatableOptional(type, default_value);
		for (auto * arg : arguments)
		{
			copy->arguments.append(arg->DeepCopy());
		}
		return copy;
	}

	bool IsSatisfied() override
	{
		return arguments.empty()
			|| arguments[arguments.size()-1]->ParametersSatisfied();
	}

	ErrorOr<Value> Evaluate(CommandContext & context) override;

	ErrorOr<Repeatable<Value> > EvaluateRepeatable(CommandContext & context) override;
}

struct OneOf : CommandParameter
{
	std::vector<std::unique_ptr<CommandParameter> > possibilities;
	int chosen_index;

	OneOf(std::vector<OwnedPtr<CommandParameter> > possibilities)
		: possibilities(possibilities)
		, chosen_index(-1)
	{ }

	std::string CommandElement::GetPrintString(std::string line_prefix) override
	{
		std::string print_string = "";
		if (chosen_index != -1)
		{
			print_string += possibilities[chosen_index]->GetPrintString(line_prefix);
		}
		// @Incomplete: default value for one of the possibilities
		return print_string;
	}

	std::unique_ptr<CommandParameter> DeepCopy() override
	{
		std::vector<std::unique_ptr<CommandParameter> > options_copy;
		for (auto && option : possibilities)
		{
			options_copy.push_back(option->DeepCopy());
		}
		auto * copy = new OneOf(options_copy);
		copy.chosen_index = chosen_index;
		return copy;
	}

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
