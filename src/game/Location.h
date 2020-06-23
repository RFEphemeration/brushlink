#pragma once
#ifndef BRUSHLINK_LOCATION_H
#define BRUSHLINK_LOCATION_H

#include <vector>
#include <variant>

#include "BuiltinTypedefs.h"
#include "ContainerExtensions.hpp"

#include "Basic_Types.h"

namespace Brushlink
{

using namespace Farb;

struct Point
{
	int x = 0;
	int y = 0;

	inline int CardinalDistance(Point other)
	{
		return abs(other.x - x) + abs(other.y - y);
	}

	inline bool IsNeighbor(Point other)
	{
		return abs(other.x - x) <= 1
			&& abs(other.y - y) <= 1
			&& (other.y != y || other.x != x);
	}

	inline bool IsCardinalNeighbor(Point other)
	{
		return CardinalDistance(other) == 1;
	}

	inline std::vector<Point> GetNeighbors()
	{
		std::vector<Point> neighbors;
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				if (i == 0 && j == 0)
				{
					 continue;
				}
				neighbors.push_back({x+i, y+j});
			}
		}
		return neighbors;
	}

	bool operator==(const Point & other) const
	{
		return x == other.x && y == other.y;
	}

	Point operator-(const Point & other) const
	{
		return Point{x - other.x, y - other.y};
	}

	Point operator+(const Point & other) const
	{
		return Point{x + other.x, y + other.y};
	}
};

} // namespace Brushlink

namespace std
{

template<>
struct hash<Brushlink::Point>
{
	std::size_t operator()(Brushlink::Point const& p) const noexcept
	{
		return std::hash<std::tuple<int,int>>{}(std::tuple{p.x, p.y});
	}
};

} // namespace std

namespace Brushlink
{

struct Line
{
	// direction: front() -> back()
	std::vector<Point> points;
};

struct Area
{
	// discrete world space
	Set<Point> points;

	static Area Circle(Point center, float radius);

	void UnionWith(const Area & other);
};

// interpreted as a vector. Different type from Point for 
struct Direction
{
	int x = 0;
	int y = 0;
};

using Location = std::variant<Point, Line, Direction, Area>;

} // namespace Brushlink

#endif // BRUSHLINK_LOCATION_H
