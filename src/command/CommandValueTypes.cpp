#include "CommandValueTypes.hpp"

#include <cmath>

namespace Command
{


Area_Interface::std::vector<Point> GetPointDistributionInArea(Number count) const
{
	const int width = (bottomRight.x - topLeft.x);
	const int height = (topLeft.y - bottomRight.y);

	const double area = GetArea();
	const double max_dimension = GetLargestDimension();
	const Point center = GetCenter();

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

double Box::GetLargestDimension() const
{
	return static_cast<double>(std::max(bottomRight.x - topLeft.x, topLeft.y - bottomRight.y));
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

double Circle::GetLargestDimension() const
{
	return static_cast<double>(radius.value * 2.0);
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

double Perimeter::GetLargestDimension() const
{
	// @Incomplete this assumes convex, I think. and equidistant perimeter points
	int count = perimeter.size();
	if (count == 0)
	{
		return 0.0;
	}
	double max_squared = 0.0;
	for (int i = 0; i < count / 2; ++i)
	{
		Point a = perimeter[i];
		Point b = perimeter[(i + count) % count];
		double distance_squared = (a - b).MagnitudeSquared();
		if (distance_squared > max_squared)
		{
			max_squared = distance_squared;
		}
	}
	return std::sqrt(max_squared);
}


function polygonArea(X, Y, numPoints) 
{ 
area = 0;   // Accumulates area 
j = numPoints-1; 

for (i=0; i<numPoints; i++)
{ area +=  (X[j]+X[i]) * (Y[j]-Y[i]); 
  j = i;  //j is previous vertex to i
}
  return area/2;
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

struct Area_Intersection : Area_Interface
{
	std::vector<std::unique_ptr<Area_Interface>> areas;
};

} // namespace Command

