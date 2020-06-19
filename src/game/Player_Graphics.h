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

	bool operator==(const Player_Graphics & other) const
	{
		return pattern == other.pattern
			&& Pack(palette.light) == Pack(other.palette.light)
			&& Pack(palette.mid) == Pack(other.palette.mid)
			&& Pack(palette.dark) == Pack(other.palette.dark);
	}
};


} // namespace Brushlink

namespace std
{

template<>
struct hash<Brushlink::Player_Graphics>
{
	std::size_t operator()(Brushlink::Player_Graphics const& pg) const noexcept
	{
		return std::hash<std::tuple<int,int,int,int> >{}(std::tuple{
			static_cast<int>(pg.pattern),
			Farb::UI::Pack(pg.palette.light),
			Farb::UI::Pack(pg.palette.mid),
			Farb::UI::Pack(pg.palette.dark),
		});
	}
};

} // namespace std


#endif // BRUSHLINK_PLAYER_GRAPHICS_H
