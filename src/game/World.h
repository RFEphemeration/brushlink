#pragma once
#ifndef BRUSHLINK_WORLD_H
#define BRUSHLINK_WORLD_H

#include "BuiltinTypedefs.h"
#include "TigrExtensions.h"

#include "Game_Basic_Types.h"
#include "Unit.h"
#include "Location.h"
#include "Player_Graphics.h"


namespace Brushlink
{

using namespace Farb;
using namespace UI;

struct World_Settings
{
	int tile_px = 16;
	std::pair<TPixel, TPixel> checker_colors {
		TPixel{192, 192, 192, 255},
		TPixel{96, 96, 96, 255}
	};
	TPixel fog_color{0,0,0,64};
	int width = 20;
	int height = 20;
};

enum class Space_Occupation
{
	Empty,
	Leaving,
	Entering,
	Full,
};

struct World
{
	World_Settings settings;
	Area area;
	std::shared_ptr<Tigr> drawn_terrain;
	Map<UnitID, Unit> units; // intentionally an ordered map for traversal
	Map<Point, UnitID> positions;
	Map<PlayerID, Player_Graphics > player_graphics;

	std::shared_ptr<Tigr> energy_bars;

	World(const World_Settings & settings = World_Settings{});

	void Render(Tigr* screen, Dimensions screen_space, Point camera_bottom_left, PlayerID player);

	bool AddUnit(Unit && unit, Point position);

	void RemoveUnit(UnitID id);

	void RemoveUnits(Set<UnitID> ids);

	Unit * GetUnit(UnitID id);

	bool MoveUnit(UnitID id, Point destination);

};

} // namespace Brushlink

#endif // BRUSHLINK_WORLD_H
