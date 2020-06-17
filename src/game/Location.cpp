#include "Location.h"

namespace Brushlink
{

Area Area::Circle(Point center, Number radius)
{
	Area a;
	a.points.insert(center);
	int radius_sqaured = radius.value * radius.value;
	for(int x = -radius.value; x <= radius.value; x++)
	{
		for (int y = -radius.value; y <= radius.value; y++)
		{
			if (x * x + y * y > radius_squared)
			{
				continue;
			}
			a.points.emplace(center.x + x, center.y + y);
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
