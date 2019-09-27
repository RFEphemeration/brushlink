#ifndef BRUSHLINK_COMMAND_TOKENS_H
#define BRUSHLINK_COMMAND_TOKENS_H

namespace Command
{

enum class TokenType
{
	Condition =             1,
	Action =                2,
	Selector =              4,
	Selector_Base =         8,
	Selector_Superlative =  16,
	Selector_Generic =      32,
	Location =              64,
	Point =                 128,
	Line =                  256,
	Area =                  512,
	Keyword =               1024, // skip, terminate, beginword, endword
	User_Defined =          2048
}

struct Token
{
	// this is a bit field of flags, aka a set of types
	TokenType type;
	HString name;

	bool IsType(TokenType other)
	{
		return other & type;
	}
}


} // namespace Command

#endif // BRUSHLINK_COMMAND_TOKENS_H