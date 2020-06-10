
#include "Location.h"

struct World_Settings
{
	int tile_px = 16;
	std::pair<TPixel, TPixel> checker_colors {
		TPixel{180, 180, 180, 255},
		TPixel{70, 70, 70, 255}
	};
	int width = 20;
	int height = 20;
};

struct World
{
	World_Settings world_settings;
	Area area;
	std::shared_ptr<Tigr> drawn_terrain;
	Map<UnitID, Unit> units;

	World(World_Settings & settings = World_Settings{});

	Render(Tigr* camera, Point camera_bottom_left, PlayerID player);
};