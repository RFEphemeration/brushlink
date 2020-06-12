

struct Point
{
	int x = 0;
	int y = 0;

	int CardinalDistance(Point other)
	{
		return abs(other.x - x) + abs(other.y - y);
	}

	bool IsNeighbor(Point other)
	{
		return abs(other.x - x) <= 1
			&& abs(other.y - y) <= 1
			&& (other.y != y || other.x != x);
	}

	bool IsCardinalNeighbor(Point other)
	{
		return CardinalDistance(other) == 1;
	}
};

struct Line
{
	// direction: front() -> back()
	std::vector<Point> points;
};

struct Area
{
	// discrete world space
	Set<Point> points;
};

// interpreted as a vector. Different type from Point for 
struct Direction
{
	int x = 0;
	int y = 0;
};

using Location = std::variant<Point, Line, Direction, Area>;