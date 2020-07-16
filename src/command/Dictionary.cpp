#include "Dictionary.h"
#include "Parser.hpp"

namespace Command
{

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

#ifndef DECLARE_CAST_BUILTIN
#define DECLARE_CAST_BUILTIN(T)	builtin(&Keywords::CastTo<T>, \
	"Builtin CastTo" #T " " #T R"( Global
	Parameter value Any)");
#endif

const Dictionary builtins = []
{
	Parser parser;
	auto builtin_print = [&](auto * func, PrintFunction print_func, std::string decl)
	{
		return parser.ParseAndAddDeclaration(
			CHECK_RETURN(parser.Lex(decl)),
			func,
			print_func);
	};
	auto builtin = [&](auto * func, std::string decl)
	{
		return parser.ParseAndAddDeclaration(
			CHECK_RETURN(parser.Lex(decl)),
			func,
			nullptr);
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
	builtin_print(&NumberLiteral::Evaluate,
		&NumberLiteral::Print,
		R"(Builtin NumberLiteral Number Global
	Parameter Repeatable Digit)");

	builtin(&Keywords::Sequence,
		R"(Builtin Sequence Any Global
	Parameter expressions Repeatable Optional  Any
		Success)");
	builtin(&Keywords::Repeat,
		R"(Builtin Repeat Any Global
	Parameter count Number
	Parameter index Optional ValueName
		"index"
	Parameter expression Any)");
	builtin(&Keywords::ForEach,
		R"(Builtin ForEach Any Global
	Parameter values Repeatable Optional Any
	Parameter iter Optional ValueName
		"iter"
	Parameter expression Any)");
	builtin(&Keywords::ForEachUnit,
		R"(Builtin ForEachUnit Any Global
	Parameter units UnitGroup
	Parameter unit Optional ValueName
		"unit"
	Parameter expression Any)");
	builtin(&Keywords::ForEachPoint,
		R"(Builtin ForEachPoint Any Global
	Parameter set OneOf
		Parameter Line
		Parameter Area
	Parameter point Optional ValueName
		"point"
	Parameter expression Any)");
	builtin(&Keywords::If,
		R"(Builtin If Any Global
	Parameter choice Bool
	Parameter primary Any
	Parameter secondary Any Optional
		Success)");
	builtin(&Keywords::IfError,
		R"(Builtin IfError Any Global
	Parameter check Any
	Parameter on_error Any
	Parameter on_value Any Optional
		check)");
	builtin(&Keywords::While,
		R"(Builtin While Any Global
	Parameter condition Bool
	Parameter expression Any)");
	DECLARE_CAST_BUILTIN(Success)
	DECLARE_CAST_BUILTIN(Bool)
	DECLARE_CAST_BUILTIN(Number)
	DECLARE_CAST_BUILTIN(Digit)
	DECLARE_CAST_BUILTIN(ValueName)
	DECLARE_CAST_BUILTIN(Letter)
	DECLARE_CAST_BUILTIN(Seconds)
	DECLARE_CAST_BUILTIN(Action_Type)
	DECLARE_CAST_BUILTIN(Unit_Type)
	DECLARE_CAST_BUILTIN(Unit_Attribute)
	DECLARE_CAST_BUILTIN(UnitID)
	DECLARE_CAST_BUILTIN(Unit_Group)
	DECLARE_CAST_BUILTIN(Energy)
	DECLARE_CAST_BUILTIN(Point)
	DECLARE_CAST_BUILTIN(Direction)
	DECLARE_CAST_BUILTIN(Line)
	DECLARE_CAST_BUILTIN(Area)

	builtin(&Context::Get,
		R"(Builtin Get Any Context
	Parameter name ValueName)");
	builtin(&Context::SetLocal,
		R"(Builtin SetLocal Any Context
	Parameter name ValueName
	Parameter value Any)");
	builtin(&Context::SetGlobal,
		R"(Builtin SetGlobal Any Context
	Parameter name ValueName
	Parameter value Any)");

	// command groups could just be a stored value
	// but that doesn't play well with our type system in the current state...

	builtin(&Context::CurrentSelection,
		R"(Builtin CurrentSelection UnitGroup Context)");
	builtin(&Context::Allies,
		R"(Builtin Allies UnitGroup Context)");
	builtin(&Context::Enemies,
		R"(Builtin Enemies UnitGroup Context)");
	builtin(&Context::CommandGroup,
		R"(Builtin CommandGroup UnitGroup Context
	Parameter id Number)");

	builtin(&Context::Select,
		R"(Builtin Select Success Context
	Parameter group OneOf
		Parameter UnitGroup
		Parameter Implied UnitGroup
			Allies)");
	builtin(&Context::SetCommandGroup,
		R"(Builtin AssignCommandGroup Success Context
	Parameter group UnitGroup Optional
		CurrentSelection
	Parameter id Number)");
	builtin(&Context::SetAction,
		R"(Builtin SetAction Success Context
	Parameter group UnitGroup Optional
		CurrentSelection
	Parameter action Action_Step)");

	builtin(&Context::Command,
		R"(Builtin Command Success Context
	Parameter command OneOf
		Parameter select Implied
			Select
		Parameter select_command_group Implied
			Select CommandGroup
		Parameter action Implied
			SetAction CurrentSelection
		Parameter statement Success # what about set value, does that return Success?
	)");
	/*
	# we no longer have a command type Action, just the variant type Success
	so this implied option would never be used
	because they accept 
		Parameter action Implied
			AssignAction
		
	*/

	return parser.parsed_definitions;
}();

#undef DECLARE_CAST_BUILTIN

} // namespace Command
