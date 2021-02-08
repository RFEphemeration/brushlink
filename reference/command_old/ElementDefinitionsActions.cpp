#include "ElementDefinitions.h"

ErrorOr<Action> set_current_selection(
	const CommandContext & context,
	UnitGroup actors)
{
	return CurriedFunctor
	{
		MemberFunction{CommandContext::SetCurrentSelectedUnits}
		, actors
	};
}


ErrorOr<Action> assign_command_group(
	const CommandContext & context,
	UnitGroup actors,
	Number group)
{
	return CurryMany(
		MemberFunction{CommandContext::AssignCommandGroup},
		actors,
		group);
}

ErrorOr<Action> add_to_command_group(
	const CommandContext & context,
	UnitGroup actors,
	Number group)
{
	return CurryMany(
		MemberFunction{CommandContext::AddToCommandGroup},
		actors,
		group);
}

ErrorOr<Action> move(
	const CommandContext & context,
	UnitGroup actors,
	Location target)
{
	return CurryMany(
		MemberFunction{CommandContext::Move},
		actors,
		target);
}

ErrorOr<Action> follow(
	const CommandContext & context,
	UnitGroup actors,
	Line target)
{
	return CurryMany(
		MemberFunction{CommandContext::Follow},
		actors,
		target);
}

ErrorOr<Action> attack(
	const CommandContext & context,
	UnitGroup actors,
	Selector target)
{
	return CurryMany(
		MemberFunction{CommandContext::Attack},
		actors,
		target);
}

ErrorOr<Action> fire_at(
	const CommandContext & context,
	UnitGroup actors,
	Location target)
{
	return CurryMany(
		MemberFunction{CommandContext::FireAt},
		actors,
		target);
}

ErrorOr<Action> cast(
	const CommandContext & context,
	UnitGroup actors,
	Ability_Type ability // what if abilities have different input requirements?
	Location target)
{
	return CurryMany(
		MemberFunction{CommandContext::Cast},
		actors,
		ability,
		target);
}

