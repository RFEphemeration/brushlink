

#include "Resources.h"
#include "Player_Graphics.h"

struct UnitIDTag
{
	static HString GetName() { return "UnitID"; }
};
using UnitID = NamedType<int, UnitIDTag>;

struct UnitGroup
{
	std::vector<UnitID> members;
};

enum class Unit_Type
{
	Spawner,
	Healer,
	Attacker,
};

Unit_Type GetRandomUnitType(std::mt19937 generator);


struct Unit_Settings
{
	Unit_Type type;
	Energy starting_energy {6};
	Energy max_energy {12};
	std::pair<Energy, Seconds> recharge_rate {{1}, {1.0}};
	Map<std::pair<Player_Color, Player_Pattern>, std::shared_ptr<Tigr>> drawn_body;
	Map<Action_Type, Action_Settings> actions;
	int vision_radius = 4;
	Map<Action_Type, Action_Magnitude_Modifier> targeted_modifiers;
};

struct Unit
{
	Unit_Settings * type;
	UnitID id;
	PlayerID player;
	Point position;
	Energy energy;
	Ticks crowded_duration;

	Action_Event pending; // if pending.type == Idle there is no pending
	Map<Action_Type, Ticks> history;
	// Command type in unit context?
	// need an already executed type stored by value
	// and a repeatedly executed type full tree
	std::vector<Command> command_queue;
	Command idle_command;
};

