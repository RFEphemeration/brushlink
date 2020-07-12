#ifndef BRUSHLINK_DICTIONARY_H
#define BRUSHLINK_DICTIONARY_H

namespace Command
{

using Dictionary = Table<ElementName, value_ptr<Element> >;

const Dictionary builtins;

struct ParameterDefinition
{
	
};

struct ElementLiteralDefinition
{
	std::string literal_type;
	std::string literal_value;
};

struct ElementDefinition
{
	std::string name;
	std::string type;
	std::optional<ParameterDefinition> left_parameters;
	std::vector<ParameterDefinition> parameters;

	std::string implementation_type;

	std::string literal_type;
	std::string literal_value;

	std::string context;
	std::string global;
	std::string print;

	std::string 
};

std::pair<ElementName, value_ptr<Element>> MakeElement(
	std::vector<Dictionary *> existing_elements,
	std::string definition);

} // namespace Command

#endif // BRUSHLINK_DICTIONARY_H