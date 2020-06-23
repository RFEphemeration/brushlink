
#include "Player_Graphics.h"

#include "TigrExtensions.h"


namespace Brushlink
{

using namespace Farb;
using namespace UI;

constexpr Palette Palette::SingleColor(TPixel mid)
{
	mid.a = 255;
	TPixel light = Tinted(mid, {255,255,255,255}, 0.25);
	light.r = DiffAndClampChannel(light.r, + 32);
	TPixel dark = Tinted(mid, {0, 0, 0, 255}, 0.25);
	dark.b = DiffAndClampChannel(dark.b, + 32);
	return Palette{{light, mid, dark}, 3, Palette_Type::SingleColor};
}

bool Player_Graphics::AcceptablyDifferentThan(
	const Player_Graphics & other,
	int palette_diff_requirement) const
{
	if (pattern == other.pattern)
	{
		// @Feature CustomPatterns
		return false;
	}
	if (palette.type != Palette_Type::SingleColor
		&& palette.type != Palette_Type::Custom
		&& other.palette.type != Palette_Type::SingleColor
		&& other.palette.type != Palette_Type::Custom)
	{
		// should we even do this? if there is a remote player with a custom palette
		// this would break
		// @Feature RemotePlayer CustomPalette
		return palette.type != other.palette.type;
	}
	int palette_min_diff_squared = 0;
	for (int i = 0; i < palette.color_count; i++)
	{
		int min_diff_squared = 255 * 255 * 4; // max possible diff squared;
		for (int j = 0; j < other.palette.color_count; j++)
		{
			int diff_squared = DiffSquared(palette.colors[i], other.palette.colors[j]);
			if (diff_squared < min_diff_squared)
			{
				min_diff_squared = diff_squared;
			}
		}
	}
	return palette_min_diff_squared >= palette_diff_requirement;
}

bool Player_Graphics::operator==(const Player_Graphics & other) const
{
	if (pattern != other.pattern
		|| palette.color_count != other.palette.color_count)
	{
		return false;
	}
	for (int c = 0; c < palette.color_count; c++)
	{
		if (Pack(palette.colors[c]) != Pack(other.palette.colors[c]))
		{
			return false;
		}
	}
	return true;
}

} // namespace Brushlink
