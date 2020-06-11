

struct Point
{
	int x = 0;
	int y = 0;

	int Distance(Point other)
	{
		return abs(other.x - x) + abs(other.y - y);
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