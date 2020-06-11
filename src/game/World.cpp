
World::World(World_Settings & settings)
	: settings(settings)
{
	const int px = settings.tile_px;
	drawn_terrain = tigrBitmap(settings.width * px, settings.height * px);

	for (int x = 0; x < settings.width; x++)
	{
		bool x_modularity = x % 6 / 3;
		for (int y = 0; y < settings.height; y++)
		{
			area.points.emplace_back(x,y);
			bool y_modularity = y % 6 / 3;
			if (x_modularity == y_modularity)
			{
				tigrFill(drawn_terrain.get(),
					x * px, y * px, px, px,
					checker_colors.first);
			}
			else
			{
				tigrFill(drawn_terrain.get(),
					x * px, y * px, px, px,
					checker_colors.second);
			}
		}
	}
}

bool World::MoveUnit(UnitID id, Point destination)
{
	Unit & unit = units[id];
	if (positions.count(destination) > 0)
	{
		return false;
	}
	positions.remove(unit.position);
	unit.position = destination;
	positions[destination] = unit.id;
}