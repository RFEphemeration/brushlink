
World::World(World_Settings & settings)
{
	const int px = settings.tile_px;

	World w;
	w.settings = settings;
	w.drawn_terrain = tigrBitmap(settings.width * px, settings.height * px);

	for (int x = 0; x < settings.width; x++)
	{
		bool x_modularity = x % 6 / 3;
		for (int y = 0; y < settings.height; y++)
		{
			w.area.points.emplace_back(x,y);
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
	return w;
}