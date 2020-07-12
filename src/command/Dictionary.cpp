#include "Dictionary.h"
#include "Parser.hpp"

namespace Command
{



std::pair<ElementName, value_ptr<Element>> MakeElement(
	std::vector<Dictionary *> existing_elements,
	std::string definition)
{
	auto token_tree = Parser::Lex(definition);

}

/*
Parameter ?Name *Flags ?Type
	?Element (for optional or implied)
	*Parameter (for oneof)
		...

Builtin Name Type (Global|Context)
	?LeftParameter
		...
	*Parameter
		...
	FunctionName
	?PrintFunctionName

Function Name Type
	?LeftParameter
		...
	*Parameter
		...
		
	+Element (implementation)
*/

const Dictionary builtins = []
{
	Parser parser;
	auto builtin = [&](std::string decl, auto func, PrintFunction print_func)
	{
		return parser.ParseAndAddDeclaration(
			CHECK_RETURN(parser.Lex(decl)),
			func,
			print_func);
	};
	auto function = [&](std::string decl)
	{
		return parser.ParseAndAddDeclaration(
			CHECK_RETURN(parser.Lex(decl)));
	}
	auto literal = [&](auto value, auto name)
	{
		parser.AddLiteral(value, {name});
	};
	literal(Success{}, "Success");
	literal(Digit{0}, "Zero");
	literal(Digit{1}, "One");
	literal(Digit{2}, "Two");
	literal(Digit{3}, "Three");
	literal(Digit{4}, "Four");
	literal(Digit{5}, "Five");
	literal(Digit{6}, "Six");
	literal(Digit{7}, "Seven");
	literal(Digit{8}, "Eight");
	literal(Digit{9}, "Nine");
	builtin(R"(Builtin NumberLiteral Number Global
	Parameter Repeatable Digit
)", NumberLiteral::Evaluate, NumberLiteral::Print);
	builtin(R"(Builtin Sequence Any Global
	Parameter Optional Repeatable Any
		Success
)", Keywords::Sequences);

	return std::move(parser.parsed_definitions);
}();

} // namespace Command
