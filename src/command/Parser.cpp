#include "Parser.hpp"

namespace Command
{

TokenTree Parser::Lex(std::stringstream text)
{
	// these should be stable because we never hold onto children in the stack
	// while modifying the parent. If we did, std::vector could move them from under us
	std::vector<TokenTree *> stack;

	TokenTree tree;
	std::string line;
	stack.push_back(&tree);
	while(std::getline(text,line,'\n'))
	{
		if (line.empty())
		{
			continue;
		}
		int indentation = [&]
		{
			int index = 0;
			while (line[index] == '\t' && index < line.size())
			{
				index++;
			}
			return index;
		}();
		if (indentation > 0)
		{
			line = line.substr(indentation);
		}

		auto tokens = Split(line, ' ');
		if (tokens.empty())
		{
			continue;
		}

		while(stack.size() > indentation + 1)
		{
			stack.pop_back();
		}
		stack.back()->nested_children.emplace_back();
		stack.back()->nested_children.back().reserve(tokens.size());
		stack.push_back(stack.back()->nested_children.back());

		for (int i = 0; i < tokens.size(); i ++)
		{
			Token token;
			token.content = tokens[i];
			if (token.content.empty())
			{
				Error("Warning - Syntax: sequential spaces are not allowed between tokens").Log();
				continue;
			}
			// @Feature docstring/formatted comments
			else if (token.content[0] == '#')
			{
				// the rest of the line is a comment, ignore remaining tokens
				break;
			}
			else if (token.content[0] == '"')
			{
				if (token.content[token.content.size() - 1] != '"')
				{
					bool closing_found = false;
					for (i++; i < tokens.size(); i++)
					{
						token.content += " " + tokens[i];
						if (tokens[i].back() == '"'
							&& (tokens[i].size() < 2
								|| tokens[i][tokens[i].size() - 2] == '\\'))
						{
							closing_found = true;
							break;
						}
					}
					if (!closing_found)
					{
						Error("Warning - Syntax: Name is missing a closing \". Names cannot contain newline characters yet.").Log();
					}
				}
				token.type = TokenType::Name;
			}
			else if (std::isdigit(token.content[0]))
			{
				token.type = TokeType::Number;
			}
			else if (std::islower(token.content[0]))
			{
				token.type = TokenType::Identifier;
			}
			else if (token.content[0] == "@")
			{
				token.type = TokenType::Identifier;
				token.content.erase(token.content.begin());
			}
			else
			{
				token.type = TokenType::Element;
			}

			stack.back()->linear_tokens.push_back(token);
		}

		if (stack.back()->linear_tokens.size() == 0)
		{
			if (stack.back()->nested_children.size() > 0)
			{
				// the root node for parsing is expected to have children and no tokens
				if (stack.size() > 1)
				{
					Error("Error - Syntax: Unexpected tree non root node has children but no tokens").Log();
				}
			}
			else
			{
				stack.pop_back();
				stack.back()->nested_children.pop_back();
			}
		}
	}
	return tree;
}

ErrorOr<value_ptr<Element>> Parser::ParseElementTree(const TokenTree & tree)
{
	Node root;
	std::vector<Node *> node_stack{};

	if (tree.linear_tokens.empty())
	{
		root = context.GetNode("ParseRoot");
		node_stack.push_back(&root);
	}

	for (auto token & : tree.linear_tokens)
	{
		Node node = CHECK_RETURN([&] -> ErrorOr<Node>
		{
			switch(token.type)
			{
			case TokenType::Identifier:
				// used in context of access and assign, how to distinguish?
				return context.MakeAccessor(token.contents);
			case TokenType::Element:
				return context.GetNode(token.contents);
			case TokenType::Name:
				return context.MakeLiteral(ValueType::String, token.contents);
			case TokenType::Number:
				auto [int_value, success] = token.ExtractInt();
				if (success)
				{
					return context.MakeLiteral(ValueType::Int, int_value);
				}
				auto [float_value, success] = token.ExtractFloat();
				if (success)
				{
					return context.MakeLiteral(ValueType::Float, float_value);
				}
				return Error("Couldn't parse expected number as either int or float");
			}
		}());

		if (node_stack.size() == 0)
		{
			root = node;
			node_stack.push_back(&root);
		}
		else
		{
			bool appended = false;
			for(;!node_stack.empty(); node_stack.pop_back())
			{
				// this can modify node_stack, increasing the stack depth for future tokens
				auto result = node_stack.back()->SetArgument(node);
				if (!result.IsError())
				{
					appended = true;
					node_stack.insert(
						node_stack.end(),
						result.GetValue().begin(),
						result.GetValue().end()
					);
					break;
				}
			}
			if (!appended)
			{
				return Error("Couldn't place node in expected place " + token.contents);
			}
		}
	}

	for (const auto & child_tree : tree.nested_children)
	{
		// @Feature implied child options from node_stack.back() that are applicable
		// in this context only
		auto node = CHECK_RETURN(Parse(context, child_tree));
		// these children are intended to be arguments to the last appended element
		// @Feature maybe it should also be okay for them to be args to the first element?
		// but not in between
		CHECK_RETURN(node_stack.back()->SetArgument(node, Node::AllowLeft));
	}

	return root;
}

const Set<Token> types {
	{TokenType::Element, "Success"},
	{TokenType::Element, "Number"},
	{TokenType::Element, "Digit"},
	{TokenType::Element, "ValueName"},
	{TokenType::Element, "Letter"},
	{TokenType::Element, "Seconds"},
	{TokenType::Element, "Action_Type"},
	{TokenType::Element, "Action_Step"},
	{TokenType::Element, "Unit_Type"},
	{TokenType::Element, "Unit_Attribute"},
	{TokenType::Element, "UnitID"},
	{TokenType::Element, "Unit_Group"},
	{TokenType::Element, "Energy"},
	{TokenType::Element, "Point"},
	{TokenType::Element, "Direction"},
	{TokenType::Element, "Line"},
	{TokenType::Element, "Area"},
};
const Token builtin {TokenType::Element, "Builtin"};
const Token global {TokenType::Element, "Global"};
const Token context {TokenType::Element, "Context"};
const Token function {TokenType::Element, "Function"};
const Token left_parameter {TokenType::Identifier, "LeftParameter"};
const Token parameter {TokenType::Element, "Parameter"};
const Token repeatable {TokenType::Element, "Repeatable"};
const Token optional {TokenType::Element, "Optional"};
const Token implied {TokenType::Element, "Implied"};
const Token oneof {TokenType::Element, "OneOf"};

const Set<Token> parameter_flags {
	repeatable,
	optional,
	implied,
	oneof
};

ErrorOr<value_ptr<Parameter>> Parser::ParseParameter(const TokenTree & tree)
{
	if (tree.linear_tokens.size() == 0)
	{
		return Error("Empty token tree");
	}
	/*
	Parameter ?Name *Flags ?Type
		?Element (for optional or implied)
		*Parameter (for oneof)
			...
	*/
	enum class Phase
	{
		Declaration,
		Name,
		Flags,
		Type,
		Element,
		Children,
		Done
	};
	Phase phase = Phase::Declaration;
	const TokenTree * branch = &tree;
	const Token * token;
	int nested_index = -1;
	int token_index = 0;

	std::optional<ValueName> name;
	Set<Token> flags_present;
	Variant_Type type {Variant_Type::Unknown};
	value_ptr<Element> element;
	std::vector<value_ptr<Parameter>> children;

	while (phase < Phase::Done)
	{
		if (nested_index < tree.nested_children.size())
		{
			branch = &tree.nested_children[nested_index];
		}
		if (token_index >= branch->linear_tokens.size())
		{
			nested_index++;
			if (nested_index >= tree.nested_children.size())
			{
				if (phase == Phase::Element
					|| phase == Phase::Parameters)
				{
					// these are not required
					break;
				}
				return Error("Parameter definition is missing required elements");
			}
			branch = &tree.nested_children[nested_index];
			token_index = 0;
		}
		if (token_index >= branch->linear_tokens.size())
		{
			return Error("Token index went out of bounds, do we have an empty nested tree?");
		}
		token = &branch->linear_tokens[token_index];

		switch(phase)
		{
		case Phase::Declaration:
			if (*token != parameter
				&& *token != left_parameter)
			{
				return Error("Token tree is not a parameter definition");
			}
			token_index++;
			phase = Phase::Name;
			break;
		case Phase::Name:
			if (!Contains(types, *token)
				&& !Contains(parameter_flags, *token))
			{
				name.value = token->contents;
				token_index++;	
			}
			// don't consume token if this wasn't a valid name, since name is optional
			phase = Phase::Flags;
			break;
		case Phase::Flags:
			if (Contains(parameter_flags, *token))
			{
				flags_present.insert(*token);
				token_index++;
				// don't increase phase here because multiple flags
				// could occur in a row
			}
			else
			{
				phase = Phase::Type;
			}
			break;
		case Phase::Type:
			if (Contains(types, *token))
			{
				type = FromString(token.contents);
				token_index++;
			}
			if (Contains(flags_present, implied)
				|| Contains(flags_present, optional))
			{
				phase = Phase::Element;
			}
			else if (Contains(flags_present, oneof))
			{
				phase = Phase::Children
			}
			else
			{
				phase = Phase::Done
			}
			break;
		case Phase::Element:
			if (token_index != 0)
			{
				TokenTree subset = *branch;
				subset.linear_tokens.erase(
					subset.linear_tokens.begin(),
					subset.linear_tokens.begin() + token_index);
				if (nested_index == -1)
				{
					// remove all Parameter nests from subset
					// because they are not part of the element definition
					// though they shouldn't be there at all anyways...
					auto child = subset.nested_children.rbegin();
					while (child != subset.nested_children.rend()
						&& *child == parameter)
					{
						child = subset.nested_children.erase(child);
					}
				}
				element = CHECK_RETURN(ParseElementTree(subset));
				SetElementImplied(element, Implicit::Child);
			}
			else
			{
				element = CHECK_RETURN(ParseElementTree(*branch));
				SetElementImplied(element, Implicit::Child);
			}
			nested_index++;
			phase = Phase::Done;
			break;
		case Phase::Parameters:
			if (*token == parameter)
			{
				if (token_index != 0 || nested_index == -1)
				{
					return Error("OneOf child parameters must be on new lines");
				}
				children.push_back(CHECK_RETURN(ParseParameter(*branch)));
				nested_index++;
			}
			else
			{
				return Error("OneOf has extra arguments after child parameters");
				phase = Phase::Done;
			}
			break;
		}
	}
	if (type == Variant_Type::Unknown
		&& element)
	{
		type = element->type;
	}

	int flag_count = flags_present.size();

	if (flag_count == 0)
	{
		// single_required
		return {new Parameter_Basic<false, false>{
			{name},
			type,
			element,
			{} // arguments start empty
		}};
	}
	else if (flag_count == 2)
	{
		if (!Contains(flags_present, optional)
			|| !Contains(flags_present, repeatable))
		{
			return Error("Unexpected parameter flag combination");
		}
		return {new Parameter_Basic<true, true>{
			{name},
			type,
			element,
			{} // arguments start empty
		}};
	}
	else if (flag_count > 2)
	{
		return Error("Unexpected parameter flag combination");
	}
	else if (Contains(flags_present, repeatable))
	{
		return {new Parameter_Basic<true, false>{
			{name},
			type,
			element,
			{} // arguments start empty
		}};
	}
	else if (Contains(flags_present, optional))
	{
		return {new Parameter_Basic<false, true>{
			{name},
			type,
			element,
			{} // arguments start empty
		}};
	}
	else if (Contains(flags_present, implied))
	{
		return {new Parameter_Implied{
			{name},
			element
		}};
	}
	else if (Contains(flags_present, oneof))
	{
		return {new Parameter_OneOf{
			{name},
			children,
			std::nullopt // chosen index starts empty
		}};
	}
	return Error("Unable to find appropriate parameter class type");
}

ErrorOr<Success> Parser::ParseDeclaration(const TokenTree & tree)
{
	/*
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
	if (tree.linear_tokens.size() == 0)
	{
		return Error("Empty token tree");
	}
	enum class Phase
	{
		Declaration,
		Name,
		Type,
		BuiltinType,
		LeftParameter,
		Parameters,
		Elements,
		Done
	};
	Phase phase = Phase::Declaration;
	const TokenTree * branch = &tree;
	const Token * token;
	int nested_index = -1;
	int token_index = 0;

	Set<Token> flags_present;

	ElementName name;
	Variant_Type type = Variant_Type::Unknown;
	value_ptr<Parameter> left_parameter;
	std::vector<value_ptr<Parameter>> parameters;
	value_ptr<Element> implementation;

	while (phase < Phase::Done)
	{
		if (nested_index < tree.nested_children.size())
		{
			branch = &tree.nested_children[nested_index];
		}
		if (token_index >= branch->linear_tokens.size())
		{
			nested_index++;
			if (nested_index >= tree.nested_children.size())
			{
				if (phase == Phase::Element
					|| phase == Phase::Parameters)
				{
					// these are not required
					break;
				}
				return Error("Parameter definition is missing required elements");
			}
			branch = &tree.nested_children[nested_index];
			token_index = 0;
		}
		if (token_index >= branch->linear_tokens.size())
		{
			return Error("Token index went out of bounds, do we have an empty nested tree?");
		}
		token = &branch->linear_tokens[token_index];

		switch(phase)
		{
		case Phase::Declaration:
			if (*token != builtin
				&& *token != function)
			{
				return Error("Element Declaration should begin with Builtin or Function");
			}
			flags_present.insert(*token);
			token_index++;
			phase = Phase::Name;
			break;
		case Phase::Name:
			name.value = token->contents;
			token_index++;
			phase = Phase::Type;
			break;
		case Phase::Type:
			if (!Contains(types, *token))
			{
				return Error("Element Declaration missing Type");
			}
			type = FromString(token->contents);
			token_index++;
			if (Contains(flags_present, builtin))
			{
				phase = Phase::BuiltinType;
			}
			else
			{
				phase = Phase::LeftParameter;
			}
			break;
		case Phase::BuiltinType:
			if (*token != global
				&& *token != context)
			{
				return Error("Builtins must define whether they are Global or Context functions");
			}
			flags_present.insert(*token);
			break;
		case Phase::LeftParameter:
			if (*token == left_parameter)
			{
				if (token_index != 0  || nested_index == -1)
				{
					return Error{"LeftParameter is expected to be on a newline"};
				}
				left_parameter = CHECK_RETURN(ParseParameter(*branch));
				nested_index++;
			}
			phase = Phase::Parameters;
			break;
		case Phase::Parameters:
			if (*token == parameter)
			{
				if (token_index != 0 || nested_index == -1)
				{
					return Error{"Parameters are expected to be on newlines"};
				}
				parameters.push_back(CHECK_RETURN(ParseParameter(*branch)));
				nested_index++;
			}
			if (Contains(flags_present, builtin))
			{
				phase = Phase::Done;
			}
			else
			{
				phase = Phase::Elements;
			}
			break;
		case Phase::Elements:
			if (token_index != 0 || nested_index == -1)
			{
				return Error{"Elements should be on newlines"};
			}
			value_ptr<Element> element = CHECK_RETURN(ParseElementTree(*branch));
			if (!implementation)
			{
				implementation.swap(element);
			}
			else
			{
				if (implementation.name.value != "Sequence")
				{
					value_ptr<Element> sequence = CHECK_RETURN(GetElement({"Sequence"}));
					auto result = sequence.Emplace(std::move(implementation));
					if (result.IsError() || implementation)
					{
						return Error{"Failed to convert multiple elements to a sequence"};
					}
					implementation.swap(sequence);
				}
				auto * param = dynamic_cast<Parameter_Basic<true, true>>(implementation->parameters.front().get());
				if (param == nullptr)
				{
					return Error{"Sequence's parameter was of an unexpected type"};
				}
				param->arguments.push_back(std::move(element));
			}
			nested_index++;
			break;
		case Phase::Done:
			break;
		}
	}
		Type decl_type;
	Declaration::Type decl_type;
	if (Contains(flags_present, context))
	{
		decl_type = Declaration::Type::Context;
	}
	else if (Contains(flags_present, global))
	{
		decl_type = Declaration::Type::Global;
	}
	else if (Contains(flags_present, function))
	{
		decl_type = Declaration::Type::Element;
	}
	else
	{
		return Error{"Unexpected flag combination in Element Declaration"};
	}
	return Declaration {
		decl_type,
		name,
		type,
		left_parameter,
		parameters,
		implementation
	};
}



} // namespace Command
