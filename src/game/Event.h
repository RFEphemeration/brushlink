
// an Event system will be a lot of work and doesn't seem worth it at this time
// reconsider after having other systems working
// until then just use conditionals every tick
/*
struct EventNameTag
{
	static HString GetName() { return "EventName"; }
};
using EventName = NamedType<HString, EventNameTag>;


struct EventValueNameTag
{
	static HString GetName() { return "EventValueName"; }
};
using EventValueName = NamedType<HString, EventValueNameTag>;

struct Event
{
	EventName name;
	UnitID actor;
	std::pair<Unit, Unit> actor_before_after;
	std::optional<UnitID> target;
	std::optional<std::pair<Unit, Unit>> target_before_after;
	Map<EventValueName, Value> values;
};
*/
