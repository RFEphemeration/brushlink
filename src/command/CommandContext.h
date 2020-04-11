
struct CommandElement;


struct CommandContext
{
	UnitGroup current_selection;

	std::list<UnitGroup> actors_stack;

	std::unique_ptr<CommandElement> command;

	std::unordered_map<HString, std::unique_ptr<CommandElement> > element_dictionary;

	void InitElementDictionary();

	void HandleToken(ElementToken token);
	ErrorOr<std::unique_ptr<CommandElement> > GetNewCommandElement(HString name);

	void PushActors(UnitGroup group);
	void PopActors();

	// Action
	void Select(UnitGroup units);
	void Move(UnitGroup actors, Location target);
	void Attack(UnitGroup actors, UnitGroup target);
	void SetCommandGroup(UnitGroup actors, Number group);
	void AddToCommandGroup(UnitGroup actors, Number group);
	void RemoveFromCommandGroup(UnitGroup actors, Number group);

	// Set
	UnitGroup Enemies();
	UnitGroup Allies();
	UnitGroup CurrentSelection();
	UnitGroup Actors();
	UnitGroup CommandGroup(Number group);
	
	// Filter, can have many
	UnitGroup WithinActorsRange(Number distance_modifier, UnitGroup set);
	UnitGroup OnScreen(UnitGroup set);

	// Group_Size, up to one
	UnitGroup GroupSize(Number size, UnitGroup set);
	UnitGroup GroupRatio(Number ratio, UnitGroup set); // implied 1/

	// Superlative, up to one
	UnitGroup ClosestToActors(UnitGroup set);

	// Location
	Location PositionOf(UnitGroup group);
	Location MousePosition();


}