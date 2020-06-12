
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
	std::map<UnitID, Unit> units; // intentionally an ordered map for traversal
	Map<Point, UnitID> positions;

	World(World_Settings & settings = World_Settings{});

	void Render(Tigr* screen, Dimensions screen_space, Point camera_bottom_left, PlayerID player);

	bool AddUnit(Unit unit, Point loc);

	void RemoveUnit(UnitID id);

	void RemoveUnits(Set<UnitID> ids);

	Unit * GetUnit(UnitID id);

	bool MoveUnit(UnitID id, Point destination);

};