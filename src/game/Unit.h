


struct UnitIDTag
{
	static HString GetName() { return "UnitID"; }
};
using UnitID = NamedType<int, UnitIDTag>;

struct UnitGroup
{
	std::vector<UnitID> members;
};

struct EnergyTag
{
	static HString GetName() { return "Energy"; }
};
using Energy = NamedType<uint, Energy>;


enum class Unit_Type
{
	Spawner,
	Healer,
	Attacker,
};

enum class Player_Pattern
{
	Checkers,
	Stripes,
	Spots,
	Grid,
};

using Player_Color = TPixel;

struct Unit_Settings
{
	Unit_Type type;
	Energy starting_energy {6};
	Energy max_energy {12};
	std::pair<Energy, Seconds> recharge_rate {{1}, {1.0}};
	Map<std::pair<Player_Color, Player_Pattern>, Tigr> drawn_body;
	Map<Action_Type, Action_Settings> actions;
	int vision_radius = 4;
};

struct Unit
{
	Unit_Settings * type;
	UnitID id;
	PlayerID player;
	Point position;
	Energy energy;

	Action_Event pending; // if pending.type == Idle there is no pending
	std::vector<Action_Event> history;
	// Command type in unit context?
	// need an already executed type stored by value
	// and a repeatedly executed type full tree
	std::vector<Command> command_queue;
	Command idle_command;
};

