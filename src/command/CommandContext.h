
struct CommandElement;


struct CommandContext
{
	// these should probably be moved to an in-game player
	UnitGroup current_selection;
	Map<int, UnitGroup> command_groups;
	std::unordered_map<HString, std::unique_ptr<CommandElement> > element_dictionary;


	std::list<UnitGroup> actors_stack;
	std::unique_ptr<CommandElement> command;
	Map<ElementType, int> allowed_next_elements;
	int skip_count;

	void InitElementDictionary();
	ErrorOr<std::unique_ptr<CommandElement> > GetNewCommandElement(HString name);
	ErrorOr<Success> HandleToken(ElementToken token);

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