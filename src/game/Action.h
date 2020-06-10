
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
	Action_Type type;
	Energy cost;
	Energy magnitude; // for heal and attack
	
};
