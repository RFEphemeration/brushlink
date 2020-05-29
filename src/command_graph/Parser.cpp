
/*

enum class CharType
{
	Ignored,
	WhiteSpace,
	Punctuation,
	Digit,
	Letter,
	Other,
}

enum class State
{
	Tree,
	Line,
	Identifier,
	Name,
	Escaped,
	Number,
}

CharType GetType(char c)
{
	switch(c)
	{
	case '\r':
		return CharType::Ignored;
	case ' ':
	case '\t':
	case '\n':
		return CharType::WhiteSpace;
	case '\\':
	case '"':
	case '.':
	case ',':
		return CharType::Punctuation;
	case '0' ... '9':
		return CharType::Digit;
	case 'a' ... 'z':
	case 'A' ... 'Z':
		return CharType::Letter;
	default:
		return CharType::Other
	}
}

TokenTree Parser::Parse(std::stringstream text)
{
	TokenTree tree;
	// these should be stable because we never hold onto children in the stack
	// while modifying the parent. If we did, std::vector could move them from under us
	std::vector<TokenTree *> stack{&tree};
	State state = State::Tree;
	int indentation = 0;

	auto AppendTreeChild = [&stack](int indentation)
	{
		while(stack.size() > indentation + 1)
		{
			stack.pop_back();
		}
		if (indentation > stack.size())
		{
			Error("Warning - Syntax: Unexpected indentation level is too deep").Log();
		}
		stack.back()->stack.back()->nested_children.emplace_back();
		stack.push_back(stack.back()->nested_children.back());
	};

	auto BeginToken = [&state, &stack](TokenType type)
	{
		stack.back()->linear_tokens.emplace_back();
		stack.back()->linear_tokens.back()->type = type;
		switch(type)
		{
			case TokenType::Identifier:
				state = State::Identifier;
				break;
			case TokenType::Name:
				state = State::Name;
				break;
			case TokenType::Number:
				state = State::Number;
				break;
		}
	};

	auto AppendCharToCurrentToken = [&stack](char c)
	{
		stack.back()->linear_tokens.back()->contents.append(c);
	};

	auto PruneTreeIfEmpty = [&stack]
	{
		if (stack.back()->linear_tokens.size() > 0)
		{
			return;
		}
		if (stack.back()->nested_children.size() > 0)
		{
			Error("Error - Syntax: Unexpected tree node with children has no tokens").Log();
			return;
		}
		stack.pop_back();
		stack.back()->nested_children.pop_back();
	};

	auto PruneTokenIfEmpty = [&stack]
	{
		if (!stack.back()->linear_tokens.back()->contents.empty()
			|| stack.back()->linear_tokens.back()->type == TokenType::Name)
		{
			return;
		}

		stack.back()->linear_tokens.pop_back();
	}
	
	while(char c = text.get())
	{
		auto type = GetType(c);
		// universal cases
		switch(type)
		{
		case CharType::Ignored:
			continue;
		case CharType::Other:
			Error("Warning - Syntax: Unexpected character '" + c + "'. Ignoring.").Log();
			continue;
		}
		switch(state)
		{
		case State::Tree:
			switch(type)
			{
			case CharType::WhiteSpace:
				switch(c)
				{
				case '\t':
					indentation += 1;
					continue;
				case '\n':
					indentation = 0;
					continue;
				default:
					Error("Warning - Syntax: unexpected whitespace before any tokens").Log();
					continue;
				}
			default:
				PruneTreeIfEmpty();
				AppendTreeChild(indentation);
				state = State::Line;
			}
		// intentional fallthrough because we have entered the line state from tree
		case State::Line:
			switch(type)
			{
			case CharType::WhiteSpace:
				Error("Warning - Syntax: unexpected extra whitespace space between tokens").Log();
				if (c == '\n')
				{
					state = State::Tree;
					indentation = 0;
				}
				continue;
			case CharType::Digit:
				BeginToken(TokenType::Number);
				AppendCharToCurrentToken(c);
				continue;
			case CharType::Punctuation:
				if (c == '"')
				{
					BeginToken(TokenType::Name);
				}
				else
				{
					Error("Warning - Syntax: unexpected punctuation " + c).Log();
				}
				continue;
			case CharType::Letter:
				BeginToken(TokenType::Identifier);
				AppendCharToCurrentToken(c);
				continue;
			default:
				Error("Warning - Parser: unexpected state/type combination").Log();
			}
		case State::Identifier:
			switch(type)
			{
			case CharType::WhiteSpace:
				switch(c)
				{
				case '\t':
					Error("Warning - Syntax: Unexpected tab after identifier").Log();
					state = State::Line;
					continue;
				case ' ':
					state = State::Line;
					continue;
				case '\n':
					state = State::Tree;
					indentation = 0;
					continue;
				}
			case CharType::Punctuation:
				Error("Warning - Syntax: Identifiers cannot contain punctuation").Log();
				continue;
			default:
				AppendCharToCurrentToken(c);
				continue;
			}
		case State::Name:
			switch(type)
			{
			case CharType::Punctuation:
				switch(c)
				{
				case '\\':
					state = State::Escaped;
					continue;
				case '"':
					state = State::Line;
					char next = text.get();
					switch(next)
					{
					case ' ':
						continue;
					case '\n':
						state = State::Tree;
						indentation = 0;
						continue;
					default:
						Error("Warning - Syntax: unexpected character after name '" + next + "'. was expecting newline or space").Log();
						text.unget();
						continue;
					}
				default:
					AppendCharToCurrentToken(c);
					continue;
				}
				continue;
			}
		case State::Escaped:
			switch(c)
			{
			case '\\':
				AppendCharToCurrentToken(c);
				continue;
			case 't':
				AppendCharToCurrentToken('\t');
				continue;
			case 'n':
				AppendCharToCurrentToken('\n');
				continue;
			case '"':
				AppendCharToCurrentToken('"');
				continue;
			default:
				Error("Warning - Syntax: unexpected escaped character '" + c + "'.").Log();
				continue;
			}
		case State::Number:
			switch(type)
			{
			case CharType::Digit:
				AppendCharToCurrentToken(c);
				continue;
			case CharType::Punctuation:
				switch(c)
				{
				case '.':
				case ',':
					AppendCharToCurrentToken(c);
					continue;
				default:
					Error("Warning - Syntax: unexpected puncutation in Number '" + c + "'.").Log();
					continue;
				}
			default:
				Error("Warning - Syntax: unexpected character in Number '" + c + "'.").Log();
				continue;
			}
		}
	}
	return tree;
}
*/

TokenTree Lex(std::stringstream text)
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
		int indentation = [&]{
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
			else
			{
				token.type = TokenType::Identifier;
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

ErrorOr<Node> Parse(EvaluationContext& context, const TokenTree & tree)
{
	Node root = context.GetNode("ParseRoot");
	std::vector<Node *> node_stack{&root};

	int node_stack_size_for_line = node_stack.size();
	for (auto token & : tree.linear_tokens)
	{
		Node node = CHECK_RETURN([&] -> ErrorOr<Node>
		{
			switch(token.type)
			{
			case TokenType::Identifier:
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

		bool appended = false;
		for(;node_stack.size() >= node_stack_size_for_line; node_stack.pop_back())
		{
			// this can modify node_stack, increasing the stack depth for future tokens
			auto result = node_stack.back()->RecursiveAppendArgument(node, node_stack);
			if (!result.IsError())
			{
				appended = true;
				break;
			}
		}
		if (!appended)
		{
			return Error("Couldn't place node in expected place " + token.contents);
		}
	}

	for (const auto & child_tree : tree.nested_children)
	{
		auto node = CHECK_RETURN(Parse(context, child_tree));
		// force these children to be arguments to the last appended element
		CHECK_RETURN(node_stack.back()->AppendArgument(node));
	}
	return Success();
}