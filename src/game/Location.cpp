#include "Location.h"

namespace Brushlink
{

Area Area::Circle(Point center, float radius)
{
	Area a;
	a.points.insert(center);
	float radius_squared = radius * radius;
	int bounds = static_cast<int>(radius + 1.0);
	for(int x = -bounds; x <= bounds; x++)
	{
		for (int y = -bounds; y <= bounds; y++)
		{
			if (static_cast<float>(x * x + y * y) > radius_squared)
			{
				continue;
			}
			a.points.insert({center.x + x, center.y + y});
		}
	}
	return a;
}

void Area::UnionWith(const Area & other)
{
	for (auto & point : other.points)
	{
		points.insert(point);
	}
}


} // namespace Brushlink
