
enum class TokenType
{
	None,
	Identifier,
	Name,
	Number,
}

struct Token
{
	TokenType type;
	std::string contents;
};

struct TokenTree
{
	std::vector<Token> linear_tokens;
	std::vector<TokenTree> nested_children;

	TokenTree(int expected_token_count)
		: linear_tokens(expected_token_count)
		: nested_children()
	{ }
};

struct Parser
{
	static TokenTree Parse(std::string text);
};
