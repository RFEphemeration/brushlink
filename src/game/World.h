
#include "Location.h"

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
	std::map<UnitID, Unit> units; // intentionally an ordered map for traversal
	Map<Point, UnitID> positions;
	Map<PlayerID, std::pair<Player_Color, Player_Pattern> > player_colors;

	std::shared_ptr<Tigr> energy_bars;

	World(World_Settings & settings = World_Settings{});

	void Render(Tigr* screen, Dimensions screen_space, Point camera_bottom_left, PlayerID player);

	bool AddUnit(Unit unit, Point position);

	void RemoveUnit(UnitID id);

	void RemoveUnits(Set<UnitID> ids);

	Unit * GetUnit(UnitID id);

	bool MoveUnit(UnitID id, Point destination);

};