
struct CommandContext
{
	UnitGroup current_selection;

	UnitGroup actors;

	Functor * command;

	// Actions
	void SetCurrentSelection(UnitGroup units);
	void Move(UnitGroup actors, Location target);
	void Attack(UnitGroup actors, UnitGroup target);
	// Hidden, Implied Action
	void SetActors(UnitGroup group);

	// Starting Sets, up to one, default is context dependent
	UnitGroup Enemies();
	UnitGroup Allies();
	UnitGroup CurrentSelection();
	UnitGroup Actors();

	// Filters, can have many
	UnitGroup WithinActorsRange(Number distance_modifier, UnitGroup set);
	UnitGroup OnScreen(UnitGroup set);

	// Group Sizes, up to one
	UnitGroup GroupSize(Number size, UnitGroup set);
	UnitGroup GroupRatio(Number ratio, UnitGroup set); // implied 1/

	// Superlatives, up to one
	UnitGroup ClosestToActors(UnitGroup set);

	// Locations
	Location PositionOf(UnitGroup group);
	Location MousePosition();

	// Helper Functions
	void HandleToken(ElementToken token);

}