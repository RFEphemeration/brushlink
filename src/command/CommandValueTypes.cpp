#include "CommandValueTypes.hpp"

#include <cmath>

namespace Command
{


Area_Interface::std::vector<Point> GetPointDistributionInArea(Number count) const
{
	const int width = (bottomRight.x - topLeft.x);
	const int height = (topLeft.y - bottomRight.y);

	const double area = GetArea();
	const Point center = GetCenter();
	const double max_dimension = GetDistanceToFarthestPoint(center);
	
	std::unique_ptr<PointDistributionGenerator> generator;

	static const PointDistributionMethod distribution = PointDistributionMethod.Fibonacci;
	switch(distribution)
	{
		case PointDistributionMethod.Fibonacci:
			generator = new FibonacciSpiralDiscGenerator(
				count.value,
				area,
				(double)center.x,
				(double)center.y);
		case PointDistributionMethod.Grid:
			generator = new GridPointGenerator(
				count.value,
				area,
				(double)center.x,
				(double)center.y);
		case PointDistributionMethod.Random:
			generator = new RandomPointGeneratior(
				max_dimension,
				(double)center.x,
				(double)center.y);
			break;
	}

	double x;
	double y;
	std::vector<Point> points;
	while(points.size() < count.value)
	{
		generator->GetNext(x, y);
		Point point{std::nearbyint(x), std::nearbyint(y)};
		if (Contains(point))
		{
			points.push_back(point);
		}
	}
}

bool Box::Contains(Point point) const
{
	return point.x >= topLeft.x
		&& point.x <= bottomRight.x
		&& point.y <= topLeft.y
		&& point.y >= bottomRight.y;
}

Point Box::GetCenter() const
{
	return (topLeft + bottomRight) / 2;
}

double Box::GetDistanceToFarthestPoint(Point from) const
{
	double max_dist_squared = (from - bottomRight).MagnitudeSquared();
	max_dist_squared = std::max(max_dist_squared,
		(from - topLeft).MagnitudeSquared());
	max_dist_squared = std::max(max_dist_squared,
		(from - Point{bottomRight.x, topLeft.y}).MagnitudeSquared());
	max_dist_squared = std::max(max_dist_squared,
		(from - Point{topLeft.x, bottomRight.y}).MagnitudeSquared());
	return std::sqrt(max_dist_squared);
}

double Box::GetArea() const
{
	return static_cast<double>(
		(bottomRight.x - topLeft.x)
		* (topLeft.y - bottomRight.y));
}

bool Circle::Contains(Point point) const
{
	int x_diff = (point.x - center.x);
	int y_diff = (point.y - center.y);
	return (x_diff * x_diff + y_diff * y_diff)
		<= (radius.value * radius.value);
}

Point Circle::GetCenter() const
{
	return center;
}

double Circle::GetDistanceToFarthestPoint(Point from) const
{
	return static_cast<double>(radius.value + (from - center).Magnitude());
}

double Circle::GetArea() const
{
	return Pi * static_cast<double>(radius.value * radius.value);
}

struct Perimeter : Area_Interface
{
	Line perimeter;
};

bool Perimeter::Contains(Point point) const
{
	// @Incomplete this assumes convex, I think. and equidistant perimeter points
	int count = perimeter.size();
	if (count == 0)
	{
		return false;
	}
	for (int i = 0; i < count / 2; ++i)
	{
		Point a = perimeter[i];
		Point b = perimeter[(i + count) % count];
		auto result = DistanceBetweenPointAndSegmentSquared(point, a, b);
		if (!result.first)
		{
			return false;
		}
	}
	return true;
}

Point Perimeter::GetCenter() const
{
	Point average;
	if (perimeter.size() == 0)
	{
		return average;
	}

	// this is a naive center point
	// it is true if points on the perimeter are spaced approximately equally, I think
	for (auto point : perimeter)
	{
		average += point;
	}

	return average / perimeter.size();


	return center;
}

double Perimeter::GetDistanceToFarthestPoint(Point from) const
{
	// @Incomplete this assumes convex, I think. and equidistant perimeter points
	int count = perimeter.size();
	if (count == 0)
	{
		return 0.0;
	}
	double max_squared = 0.0;
	for (int i = 0; i < count; ++i)
	{
		double distance_squared = (perimeter[i] - from).MagnitudeSquared();
		if (distance_squared > max_squared)
		{
			max_squared = distance_squared;
		}
	}
	return std::sqrt(max_squared);
}

double Perimeter::GetArea() const
{
	double area = 0;
	int count = perimeter.size();
	i = count - 1;
	for (int j = 0; j < count; j++)
	{
		auto & a = perimeter[i];
		auto & b = perimeter[j];
		// this might be wrong, could it be a.x * b.y - a.y * b.x
		area += static_cast<double>(a.x + b.x)
			* static_cast<double>(a.y - b.y);
		i = j; // i trails j
	}
	return area / 2.0;
}


struct Area_Union : Area_Interface
{
	std::vector<std::unique_ptr<Area_Interface>> areas;
};

bool Area_Union::Contains(Point point) const
{
	for (auto area : areas)
	{
		if (area->Contains(point))
		{
			return true;
		}
	}
	return false;
}

Point Area_Union::GetCenter() const
{
	if (areas.size() == 0)
	{
		return Point();
	}
	double total_weight;
	Point2D<double> weighted_center;
	for (auto area : areas)
	{
		double area_weight = area->GetArea();
		Point center = area->GetCenter();

		total_weight += area_weight;
		weighted_center.x += static_cast<double>(center.x) * area_weight;
		weighted_center.y += static_cast<double>(center.y) * area_weight;
	}
	return Point{
		std::nearestint(weighted_center.x / total_weight),
		std::nearestint(weighted_center.y / total_weight)};
}

double Area_Union::GetDistanceToFarthestPoint(Point from) const
{
	if (areas.size() == 0)
	{
		return 0.0;
	}
	double max_distance = 0.0;
	for (auto area : areas)
	{
		double distance = area->GetDistanceToFarthestPoint(from);
		if (distance > max_distance)
		{
			max_distance = distance;
		}
	}
	return max_distance;
}

double Area_Union::GetArea() const
{
	// @Incomplete assumes disjoint, which we might later change
	double total_area = 0.0;
	for (auto area : areas)
	{
		total_area += area->GetArea();
	}
	return total_area;
}

} // namespace Command
