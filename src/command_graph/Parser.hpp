
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



enum class CharType
{
	Ignored,
	WhiteSpace,
	Punctuation,
	Digit,
	Letter,
	Other,
}

struct Parser
{
	enum class State
	{
		Tree,
		Line,
		Identifier,
		Name,
		Escaped,
		Number,
	}

	static CharType GetType(char c);

	static TokenTree Parse(std::string text);
};
