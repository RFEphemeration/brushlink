#pragma once
#ifndef BRUSHLINK_GAME_TIME_H
#define BRUSHLINK_GAME_TIME_H

#include "NamedType.hpp"

namespace Brushlink
{

struct TicksTag
{
	static HString GetName() { return "Ticks"; }
};
using Ticks = NamedType<int, TicksTag>;

struct SecondsTag
{
	static HString GetName() { return "Seconds"; }
};
using Seconds = NamedType<float, SecondsTag>;

/*
struct Seconds
{
	float value;

	Seconds(float value)
		: value(value)
	{ }

	Seconds(int numerator, int divisor)
		: value(static_cast<float>(numerator) / static_cast<float>(divisor))
	{ }

	Seconds()
		: value(0.0)
	{ }
};
*/


} // namespace Brushlink

#endif // BRUSHLINK_TIME_H
