
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
	// @Incomplete: if actions are handled literally
	// what do we do about appending sequential actions?
	// should we create subcontexts that store a functor
	// or should these just return a functor anyways?
	// if they do return a functor, how do we handle actors?
	// maybe actors aren't important after evaluation anyways
	// because everything has been used already
	// but that feels strange to use their current state
	// instead of their state at the end of their in progress action
	// so maybe enqueue should just store off the command element
	// instead of evaluating it right away
	// but we do need to get the actors out...
	// maybe actors should be a function of a CommandElement
	// and Actors() just asks for the most recent/distal child with actors
	void Select(UnitGroup units);
	void Move(UnitGroup actors, Location target);
	void Attack(UnitGroup actors, UnitGroup target);
	void SetCommandGroup(UnitGroup actors, Number group);
	// void AddToCommandGroup(UnitGroup actors, Number group);
	// void RemoveFromCommandGroup(UnitGroup actors, Number group);

	// Set
	UnitGroup Enemies();
	UnitGroup Allies();
	UnitGroup CurrentSelection();
	UnitGroup Actors();
	UnitGroup CommandGroup(Number group);
	
	// Filter, can have many
	Filter FilterIdentity(UnitGroup set);
	Filter WithinActorsRange(UnitGroup set, Number distance_modifier);
	Filter OnScreen(UnitGroup set);

	// GroupSize, up to one
	GroupSize GroupSizeIdentity(UnitGroup set);
	GroupSize GroupSize(UnitGroup set, Number size);
	GroupSize GroupActorsRatio(UnitGroup set, Number ratio); // implied 1/

	// Superlative, up to one
	Superlative SuperlativeRandom(UnitGroup set, Number size);
	Superlative ClosestToActors(UnitGroup set, Number size);

	// Location
	Location PositionOf(UnitGroup group);
	// Location MousePosition();


}