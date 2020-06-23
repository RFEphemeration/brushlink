#pragma once
#ifndef BRUSHLINK_PLAYER_GRAPHICS_H
#define BRUSHLINK_PLAYER_GRAPHICS_H

#include "tigr.h"
#include "ContainerExtensions.hpp"
#include "TigrExtensions.h"

namespace Brushlink
{

using namespace Farb;
using namespace UI;

enum class Pattern
{
	Checkers,
	Stripes,
	Spots,
	Grid,
};

enum class Palette_Type
{
	SingleColor = -1,
	Green,
	Blue,
	Purple,
	Orange,
	Brown,
	Grey,
	Custom
};

struct Palette
{
	TPixel colors[3]; // @Feature expandable color palettes, or at least larger max
	int color_count{3};
	Palette_Type type{Palette_Type::Purple};
	/*
	TPixel light;
	TPixel mid;
	TPixel dark;
	*/

	static constexpr Palette SingleColor(TPixel mid);
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

	bool operator==(const Player_Graphics & other) const;
};


} // namespace Brushlink

namespace std
{

template<>
struct hash<Brushlink::Player_Graphics>
{
	std::size_t operator()(Brushlink::Player_Graphics const& pg) const noexcept
	{
		size_t hash = pg.palette.color_count;
		Farb::HashCombine(hash, static_cast<int>(pg.pattern));
		for(int i = 0; i < pg.palette.color_count; i++)
		{
			Farb::HashCombine(hash, Farb::UI::Pack(pg.palette.colors[i]));
		}
		return hash;
	}
};

} // namespace std


#endif // BRUSHLINK_PLAYER_GRAPHICS_H
