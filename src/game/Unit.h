


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
	Reproduce,
	Heal,
	Melee,
};

struct Unit_Settings
{
	Unit_Type type;
	Energy max_energy = Energy{12};
	Energy starting_energy = Energy{0};
	std::pair<Ticks, Energy> recharge_rate = {Ticks{12}, Energy{1}};
	std::shared_ptr<Tigr> drawn_body;
	std::unordered_set<Action_Type, Action_Settings> actions;
	int vision_radius = 4;
};

struct Unit
{
	Unit_Settings * type;

	UnitID id;
	PlayerID player;
	Point position;
	Energy energy;
};

