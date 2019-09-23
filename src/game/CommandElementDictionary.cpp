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

ErrorOr<Number> plus(CommandContext context, Number left, Number right)
{
	return Number{a.value + b.value};
}

template<int value>
ErrorOr<Number> digit(CommandContext context, Number left)
{
	return Number{(left.value * 10) + value};
}

#define DIGIT(value) static auto digit_ ## value = ElementAtom{digit<value>, ElementParameter{ValueType::Number, Value{Number{0}}}}


void ElementDictionary::InitializeTypedAtoms()
{
	// initialize required literals first

	// these

	DIGIT(0);
	DIGIT(1);
	DIGIT(2);
	DIGIT(3);
	DIGIT(4);
	DIGIT(5);
	DIGIT(6);
	DIGIT(7);
	DIGIT(8);
	DIGIT(9);

	static auto current_selection = ElementAtom{atom_current_selection};

	
	static auto fire_at = ElementAtom{atom_fire_at, true, };
}