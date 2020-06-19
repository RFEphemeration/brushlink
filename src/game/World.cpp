
#include "World.h"

#include <algorithm>

namespace Brushlink
{

World::World(const World_Settings & settings)
	: settings(settings)
{
	const int px = settings.tile_px;
	drawn_terrain.reset(tigrBitmap(settings.width * px, settings.height * px));

	for (int x = 0; x < settings.width; x++)
	{
		bool x_modularity = x % 6 / 3;
		for (int y = 0; y < settings.height; y++)
		{
			area.points.insert({x,y});
			bool y_modularity = y % 6 / 3;
			if (x_modularity == y_modularity)
			{
				tigrFill(drawn_terrain.get(),
					x * px, y * px, px, px,
					settings.checker_colors.first);
			}
			else
			{
				tigrFill(drawn_terrain.get(),
					x * px, y * px, px, px,
					settings.checker_colors.second);
			}
		}
	}
}

void World::Render(Tigr* screen, Dimensions screen_space, Point camera_bottom_left, PlayerID player)
{
	// render background
	tigrBlit(
		screen,
		drawn_terrain.get(),
		screen_space.x,
		screen_space.y,
		camera_bottom_left.x * settings.tile_px,
		camera_bottom_left.y * settings.tile_px,
		screen_space.width,
		screen_space.height);

	// render units
	int energy_granularity = energy_bars->w / settings.tile_px;
	auto Get_Screen_Space_Offset = [&](Point position)
	{
		// todo: reconcile screen/world space up
		Point screen_space_offset = position - camera_bottom_left;
		screen_space_offset.x *= settings.tile_px;
		screen_space_offset.y *= settings.tile_px;
		return screen_space_offset;
	};

	auto Trim_Source_Dimensions = [&](
		Dimensions source_dim,
		Point screen_space_offset)
	{
		if (screen_space_offset.x < 0)
		{
			source_dim.x -= screen_space_offset.x;
			source_dim.width += screen_space_offset.x;
		}
		if (screen_space_offset.y < 0)
		{
			source_dim.y -= screen_space_offset.x;
			source_dim.width += screen_space_offset.x;
		}
		source_dim.width = std::min(source_dim.width,
			screen_space.width - screen_space_offset.x);
		source_dim.height = std::min(source_dim.height,
			screen_space.height - screen_space_offset.y);
		return source_dim;
	};

	auto Render_Unit = [&](Unit & unit)
	{
		// @Feature interpolate position while moving
		Tigr * body = unit.type->drawn_body[player_graphics[unit.player]].get();
		Point screen_space_offset = Get_Screen_Space_Offset(unit.position);
		Dimensions sprite_source = Trim_Source_Dimensions(
			Dimensions{0, 0, body->w, body->h},
			screen_space_offset
		);

		if (sprite_source.width <= 0
			|| sprite_source.height <= 0)
		{
			// this unit is not on screen, do not render
			return;
		}
		tigrBlitAlpha(
			screen,
			body,
			screen_space.x + screen_space_offset.x,
			screen_space.y + screen_space_offset.y,
			sprite_source.x,
			sprite_source.y,
			sprite_source.width,
			sprite_source.height,
			1.0);

		// energy bar
		float energy_ratio = static_cast<float>(unit.energy.value)
			/ static_cast<float>(unit.type->max_energy.value);
		int energy_index = energy_ratio * energy_granularity;
		if (energy_ratio < 0.001)
		{
			energy_index = 0;
		}
		else if (energy_index == 0)
		{
			// empty is reserved for 0 or less
			// so anything more than .001 has a little visible
			energy_index = 1;
		}
		else if (energy_ratio > 0.999)
		{
			energy_index = energy_granularity;
		}
		Dimensions energy_source = Trim_Source_Dimensions(
			Dimensions{
				energy_index * settings.tile_px,
				0,
				settings.tile_px,
				settings.tile_px // consider copying a subset of the tile
			},
			screen_space_offset
		);
		tigrBlitAlpha(
			screen,
			energy_bars.get(),
			screen_space.x + screen_space_offset.x,
			screen_space.y + screen_space_offset.y,
			energy_source.x,
			energy_source.y,
			energy_source.width,
			energy_source.height,
			1.0); 
	};

	// render friendly units
	std::vector<UnitID> non_player_units;
	Area player_vision;

	for(auto & pair : units)
	{
		Unit & unit = pair.second;
		if(unit.player != player)
		{
			non_player_units.push_back(unit.id);
			continue;
		}

		Area unit_vision = Area::Circle(unit.position, unit.type->vision_radius);
		player_vision.UnionWith(unit_vision);
		Render_Unit(unit);
	}

	// render unfriendly units
	for (auto & id : non_player_units)
	{
		Unit & unit = units[id];
		// don't render units that are out of vision range
		if (!Contains(player_vision.points, unit.position))
		{
			continue;
		}
		Render_Unit(unit);
	}

	// @Feature ability fx

	// render fog
	Point camera_top_right { camera_bottom_left.x + screen_space.width / settings.tile_px,
		camera_bottom_left.y + screen_space.height / settings.tile_px,
	};
	for (int x = camera_bottom_left.x; x <= camera_top_right.x; x++)
	{
		for (int y = camera_bottom_left.y; y <= camera_top_right.y; y++)
		{
			Point p{x, y};
			if (Contains(area.points, p) // we allow camera to pan off screen a bit
				&& !Contains(player_vision.points, p))
			{
				Point screen_space_offset = Get_Screen_Space_Offset(p);
				tigrFillTint(
					screen,
					screen_space.x + screen_space_offset.x,
					screen_space.y + screen_space_offset.y,
					settings.tile_px,
					settings.tile_px,
					settings.fog_color);
			}
		}
	}
}


bool World::AddUnit(Unit && unit, Point position)
{	
	UnitID id = unit.id;
	if (Contains(positions, position)
		|| Contains(units, id))
	{
		return false;
	}
	units[id] = unit;
	positions[position] = id;
	units[id].position = position;
	return true;
}

void World::RemoveUnit(UnitID id)
{
	if (!Contains(units, id))
	{
		return;
	}
	positions.erase(units[id].position);
	units.erase(id);
}

void World::RemoveUnits(Set<UnitID> ids)
{
	for (auto id : ids)
	{
		RemoveUnit(id);
	}
}

Unit * World::GetUnit(UnitID id)
{
	if (Contains(units, id))
	{
		return &units[id];
	}
	return nullptr;
}

bool World::MoveUnit(UnitID id, Point destination)
{
	Unit * unit = GetUnit(id);
	if (unit == nullptr
		|| positions.count(destination) > 0)
	{
		return false;
	}
	positions.erase(unit->position);
	unit->position = destination;
	positions[destination] = unit->id;
	return true;
}

} // namespace Brushlink
