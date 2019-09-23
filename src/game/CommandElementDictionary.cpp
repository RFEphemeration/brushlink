#include "CommandElementDictionary.h"

// how to handle 1-1, 2-1, or all-to-one ratios?
// in CommandContext?

/*
	CommandStatement,
	UnitGroup,
	Number,
	UnitType,
	AbilityType,
	AttributeType,		
	Point,
	Line,
	Direction,
	Area
*/

ErrorOr<UnitGroup> current_selection(CommandContext context);

// probably need separate atoms for fire at point, line, unitgroup...
// otherwise how do we find
ErrorOr<CommandStatement> fire_at(CommandContext contex, UnitGroup actor, Point target);

ErrorOr<Point> position_of(CommandContext contex, UnitGroup target);

ErrorOr<UnitGroup> units_in_area(CommandContext context, Area area);

ErrorOr<Area> circle(CommandContext context, Point center, Number radius);

ErrorOr<UnitGroup> limit_group_size(CommandContext context, Number size, UnitGroup group);

ErrorOr<UniGroup> closest(CommandContext context, UnitGroup actor, UnitGroup target);


void ElementDictionary::InitializeTypedAtoms()
{
	// initialize required literals first

	// these 

	static auto current_selection = ElementAtom{atom_current_selection};

	
	static auto fire_at = ElementAtom{atom_fire_at, true, };
}