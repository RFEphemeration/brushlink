#ifndef BRUSHLINK_ELEMENT_DEFINITIONS_HPP
#define BRUSHLINK_ELEMENT_DEFINITIONS_HPP

namespace Command
{

struct Value
{

}

struct ElementDefinition
{
	virtual ErrorOr<Value> Evaluate(
		CommandContext context,
		std::map<ParameterIndex, Value> arguments) const;
}

} // namespace Command

#endif // BRUSHLINK_ELEMENT_DEFINITIONS_HPP