#pragma once
#ifndef BRUSHLINK_PARSER_HPP
#define BRUSHLINK_PARSER_HPP

#include <type_traits>

namespace Command
{

enum class TokenType
{
	None,
	Element,
	Identifier
	Name,
	Number,
}

struct Token
{
	TokenType type;
	std::string contents;

	bool operator==(Token const& other) const
	{
		return type == other.type && contents == other.contents;
	}
};

struct TokenTree
{
	std::vector<Token> linear_tokens;
	std::vector<TokenTree> nested_children;
};

struct Declaration
{
	enum class Type
	{
		Context,
		Global,
		Element
	};

	Type decl_type;

	ElementName name;
	Variant_Type type = Variant_Type::Unknown;
	value_ptr<Parameter> left_parameter;
	std::vector<value_ptr<Parameter>> parameters;
	value_ptr<Element> implementation;
};

struct Parser
{
	std::vector<Dictionary *> existing_definitions;
	Dictionary parsed_definitions;

	static TokenTree Lex(std::stringstream text);

	ErrorOr<Success> Parse(EvaluationContext& context, const TokenTree & tree);

	ErrorOr<Declaration> ParseDeclaration(const TokenTree & tree);
	ErrorOr<value_ptr<Parameter>> ParseParameter(const TokenTree & tree) const;
	ErrorOr<value_ptr<Element>> ParseElementTree(const TokenTree & tree) const;

	template<typename F>
	ErrorOr<Success> ParseAndAddDeclaration(const TokenTree & tree, F evaluate, PrintFunction print)
	{
		Declaration decl = CHECK_RETURN(ParseDeclaration(tree));
		if constexpr(std::is_member_function_pointer<F>::value)
		{
			if (decl_type != Declaration::Type::Context)
			{
				return Error("This declaration was expected to be a Context Builtin");
			}
			value_ptr<Element> def {new ContextFunction{
				{	
					decl.name,
					decl.type,
					std::move(decl.left_parameter),
					std::move(decl.parameters)
				},
				evaluate,
				print
			}};
			parsed_definitions.insert(decl.name, std::move(defn));
		}
		else
		{
			if (decl_type != Declaration::Type::Global)
			{
				return Error("This declaration was expected to be a Global Builtin");
			}
			value_ptr<Element> def {new GlobalFunction{
				{	
					decl.name,
					decl.type,
					std::move(decl.left_parameter),
					std::move(decl.parameters)
				},
				evaluate,
				print
			}};
			parsed_definitions.insert(decl.name, std::move(defn));
		}

		return Success{};
	}

	ErrorOr<Success> ParseAndAddDeclaration(const TokenTree & tree)
	{
		Declaration decl = CHECK_RETURN(ParseDeclaration(tree));
		if (decl_type != Declaration::Type::Element)
		{
			return Error("This declaration needs a builtin function pointer");
		}
		value_ptr<Element> def {new ElementFunction{
			{	
				decl.name,
				decl.type,
				std::move(decl.left_parameter),
				std::move(decl.parameters)
			},
			std::move(implementation)
		}};
		parsed_definitions.insert(decl.name, std::move(defn));

		return Success{};
	}

	template<typename TVal>
	ErrorOr<Success> AddLiteral(TVal&& value, ElementName name)
	{
		if (name.value.empty())
		{
			return Error("Dictionary literals must have a name");
		}
		parsed_definitions.insert(literal.name, {new Literal{value, name}});
	}

	value_ptr<Element> GetElement(ElementName name) const;

}




} // namespace Command

#endif // BRUSHLINK_PARSER_HPP
