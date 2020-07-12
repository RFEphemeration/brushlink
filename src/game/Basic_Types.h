#pragma once
#ifndef BRUSHLINK_BASIC_TYPES
#define BRUSHLINK_BASIC_TYPES

#include "NamedType.hpp"

namespace Brushlink
{

using namespace Farb;

using Bool = bool;

struct NumberTag
{
	static HString GetName() { return "Command::Number"; }
};
using Number = NamedType<int, NumberTag>;

struct DigitTag
{
	static HString GetName() { return "Command::Digit"; }
};
using Digit = NamedType<int, DigitTag>;

struct ValueNameTag
{
	static HString GetName() { return "ValueName"; }
};
using ValueName = NamedType<HString, ValueNameTag>;

struct LetterTag
{
	static HString GetName() { return "Letter"; }
};
using Letter = NamedType<char, LetterTag>;

} // namespace Brushlink

#endif // BRUSHLINK_BASIC_TYPES
