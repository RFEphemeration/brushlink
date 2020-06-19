
#include "Player_Graphics.h"

#include "TigrExtensions.h"


namespace Brushlink
{

using namespace Farb;
using namespace UI;

Palette Palette::SingleColor(TPixel mid)
{
	mid.a = 255;
	TPixel light = Tinted(mid, {255,255,255,255}, 0.25);
	light.r = DiffAndClampChannel(light.r, + 32);
	TPixel dark = Tinted(mid, {0, 0, 0, 255}, 0.25);
	dark.b = DiffAndClampChannel(dark.b, + 32);
	return Palette{light, mid, dark};
}

bool Player_Graphics::AcceptablyDifferentThan(
	const Player_Graphics & other,
	int palette_diff_requirement) const
{
	if (pattern == other.pattern)
	{
		return false;
	}
	int palette_min_diff_squared = 0;
	for (auto Palette::* p : std::vector<TPixel Palette::*>{
			&Palette::light,
			&Palette::mid,
			&Palette::dark})
	{
		// consider all colors in other palette for min distance
		int min_diff_squared = DiffSquared(palette.*p, other.palette.light);
		min_diff_squared = std::min(min_diff_squared,
			DiffSquared(palette.*p, other.palette.mid));
		min_diff_squared = std::min(min_diff_squared,
			DiffSquared(palette.*p, other.palette.dark));
		palette_min_diff_squared += min_diff_squared;
	}
	return palette_min_diff_squared >= palette_diff_requirement;
}

} // namespace Brushlink
