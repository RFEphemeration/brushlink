#include "Dictionary"

namespace Command
{



std::pair<ElementName, value_ptr<Element>> MakeElement(
	std::vector<Dictionary *> existing_elements,
	std::string definition)
{
	auto token_tree = Parser::Lex(definition);

}

const Dictionary builtins = []
{
	Dictionary builtins;
	builtins.insert({});

	return builtins;
}();

} // namespace Command
