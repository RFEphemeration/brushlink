#pragma once
#ifndef BRUSHLINK_PLAYER_GRAPHICS_H
#define BRUSHLINK_PLAYER_GRAPHICS_H

#include "tigr.h"

namespace Brushlink
{

enum class Pattern
{
	Checkers,
	Stripes,
	Spots,
	Grid,
};

struct Palette
{
	TPixel light;
	TPixel mid;
	TPixel dark;

	static Palette SingleColor(TPixel mid);
};

enum class Builtin_Palette_Colors
{
	Purple,
	Blue,
	Green,
	Orange,
	Tan,
	Grey
};



struct Player_Graphics
{
	Pattern pattern;
	Palette palette;

	bool AcceptablyDifferentThan(
		const Player_Graphics & other,
		int palette_diff_requirement) const;
};


} // namespace Brushlink

#endif // BRUSHLINK_PLAYER_GRAPHICS_H
