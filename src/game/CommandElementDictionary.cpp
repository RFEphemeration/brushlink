#include "CommandElementDictionary.h"

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

ErrorOr<UnitGroup> atom_current_selection(CommandContext context);

ErrorOr<CommandStatement> atom_fire_at(CommandContext contex, UnitGroup actor, Point target);

void ElementDictionary::InitializeTypedAtoms()
{
	// initialize required literals first

	// these 

	static auto current_selection = ElementAtom{atom_current_selection};

	
	static auto fire_at = ElementAtom{atom_fire_at, true, };
}