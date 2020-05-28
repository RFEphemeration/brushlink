enum class State
{
	Tree,
	Line,
	Identifier,
	Name,
	Number,
}

TokenTree Parser::Parse(std::stringstream text)
{
	TokenTree tree;
	std::string line;
	// these should be stable because we never hold onto children in the stack
	// while modifying the parent. If we did, std::vector could move them from under us
	std::vector<TokenTree *> stack{&tree};
	if (text == nullptr)
	{
		return tree;
	}
	auto AppendTokenToTree = [&stack](const Token & token)
	{
		stack.back()->linear_children.push_back(token);
	}
	auto NavigateStackTo = [&stack](int indentation, int expected_token_count)
	{
		while(stack.size() > indentation + 1)
		{
			stack.pop_back();
		}
		stack.back()->nested_children.emplace_back();
		stack.back()->nested_children.back().reserve(expected_token_count);
		stack.push_back(stack.back()->nested_children.back());
	};
	auto PruneLineIfEmpty = [&stack]{
		if (stack.back()->linear_children.size() > 0)
		{
			return;
		}
		stack.pop_back();
		stack.back()->nested_children.pop_back();
	};
	State state = State::Tree;

	auto BeginToken = [&state, &stack](TokenType type)
	{
		stack.back()->linear_children.emplace_back();
		stack.back()->linear_children.back()->type = type;
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
		stack.back()->linear_children.back()->contents.append(c);
	};

	auto BeginTree
	int indentation = 0;
	while(char c = text.get())
	{
		switch(state)
		{
		case State::Tree:
			switch(c)
			{
			case '\t':
				indentation += 1;
				continue;
			}
			// intentional fallthrough because tree and line have a lot in common
		case State::Line:
			switch(c)
			{
			case '0' ... '9':
				BeginToken(TokenType::Number);
				AppendCharToCurrentToken(c);
				continue;
			case '"':
				BeginToken(TokenType::Name);
				continue;
			default:
				BeginToken(TokenType::Identifier);
				AppendCharToCurrentToken(c);
				continue;
			}
		case State::Identifier:
			switch(c)
			{
				case ' ':
					state = State::Line;
					continue;
				case '\n':
					state = State::Tree;
					continue;
				default:
					AppendCharToCurrentToken(c);
					continue;
			}
		case State::Name:
		case State::Number:
		}
	}
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
		line = std::substr(indentation);

		auto tokens = Split(line, ' ');
		if (tokens.empty())
		{
			continue;
		}
		NavigateStackTo(indentation, tokens.size());
		for (auto content & : tokens)
		{
			Token token;
			token.content = content;
			if (content.empty())
			{
				Error("Warning - Syntax: sequential spaces are not allowed between tokens").Log();
				continue;
			}
			else if (content[0] == '"')
			{
				if (content[content.size() - 1] != '"')
				{
					// @Feature strings with spaces
					Error("Warning - Syntax: Name " + content + " is missing closing quotation. Names with spaces are not allowed.").Log();
				}
				token.type = TokenType::Name;
			}
			else if (std::isdigit(content[0]))
			{
				token.type = TokeType::Number;
			}
			else
			{
				token.type = TokenType::Identifier;
			}
			AppendTokenToTree(token);
		}
		PruneLineIfEmpty();
	}
}