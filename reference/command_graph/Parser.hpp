
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
};

TokenTree Lex(std::stringstream text);

ErrorOr<Node> Parse(EvaluationContext& context, const TokenTree & tree);