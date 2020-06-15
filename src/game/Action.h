
enum class Action_Type
{
	Nothing,
	Idle,
	Move,
	Attack,
	Heal,
	Reproduce
};

struct Action_Settings
{
	Energy cost {0};
	Energy magnitude {0}; // for heal and attack
	Seconds cooldown {0.0};
	Seconds duration {0.0};
};

struct Action_Event
{
	Action_Type type;
	std::optional<Point> location;
	std::optional<UnitID> target;
	//Tick time;
};


enum class Action_Result
{
	Waiting,
	Success,
	Retry,
	Recompute
};


struct Action_Magnitude_Modifier
{
	Energy amount;
};
