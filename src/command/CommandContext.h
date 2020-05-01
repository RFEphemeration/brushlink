#ifndef BRUSHLINK_COMMAND_CONTEXT_H
#define BRUSHLINK_COMMAND_CONTEXT_H

#include "CommandValueTypes.hpp"
#include "ElementType.h"

#include <list>

namespace Command
{


struct CommandElement;

struct AllowedTypes
{
	// token here is used because it has type and has_left_parameter
	// but name isn't used. could consider a different struct
	std::vector<ElementToken> priority;

	Table<ElementType::Enum, int> total_right;
	Table<ElementType::Enum int> total_left;
}


struct CommandContext
{
	// these should probably be moved to an in-game player
	UnitGroup current_selection;
	Table<int, UnitGroup> command_groups;
	Table<HString, value_ptr<CommandElement> > element_dictionary;

	// these could be more levels of const but it's a pain to work with them
	// so we're just going to pretend they're not and that one const is enough
	// to communicate intention
	static const Table<ElementType::Enum, Set<ElementType::Enum>> allowed_with_implied;
	static const Table<ElementType::Enum, Table<ElementType::Enum, ElementName>> implied_elements;
	static const Set<ElementType::Enum> instruction_element_types;

	std::list<UnitGroup> actors_stack;
	value_ptr<CommandElement> command;
	Table<ElementType::Enum, int> allowed_next_elements_right;
	Table<ElementType::Enum, int> allowed_next_elements_left;
	int skip_count;
	std::vector<ElementToken> undo_stack;
	int undo_count;
	std::vector<std::string> action_log;

	void InitElementDictionary();
	ErrorOr<value_ptr<CommandElement> > GetNewCommandElement(HString name);
	ErrorOr<ElementToken> GetTokenForName(ElementName name);
	std::vector<ElementToken> GetAllTokens();
	
	ErrorOr<Success> InitNewCommand();
	void GetAllowedNextElements(Set<ElementName> & allowed);
	ErrorOr<Success> HandleToken(ElementToken token);
	bool IsAllowed(ElementToken token);

	void RefreshAllowedTypes();
	ErrorOr<Success> PerformUndo();
	ErrorOr<Success> PerformRedo();
	void BreakUndoChain(ElementToken token);
	ErrorOr<Success> AppendElement(value_ptr<CommandElement>&& next);

	void PushActors(UnitGroup group);
	void PopActors();
	std::string ToLogString(Value v);
	void LogAction(std::string entry);

	ErrorOr<Location> LocationConversion(Value value);

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
	ErrorOr<Success> Select(UnitGroup units);
	ErrorOr<Success> Move(UnitGroup actors, Location target);
	ErrorOr<Success> Attack(UnitGroup actors, UnitGroup target);
	ErrorOr<Success> SetCommandGroup(UnitGroup actors, Number group);
	// void AddToCommandGroup(UnitGroup actors, Number group);
	// void RemoveFromCommandGroup(UnitGroup actors, Number group);

	// Set
	ErrorOr<UnitGroup> Enemies();
	ErrorOr<UnitGroup> Allies();
	ErrorOr<UnitGroup> CurrentSelection();
	ErrorOr<UnitGroup> Actors();
	ErrorOr<UnitGroup> CommandGroup(Number group);
	
	// the filter, groupsize, and superlative types
	// make it difficult to allow users to define new ones
	// @Feature reconsider private/internal arguments for elements

	// Filter, can have many
	ErrorOr<Filter> WithinActorsRange(Number distance_modifier);
	ErrorOr<Filter> OnScreen();

	// GroupSize, up to one
	ErrorOr<GroupSize> GroupSizeLiteral(Number size);
	ErrorOr<GroupSize> GroupActorsRatio(Number ratio); // implied 1/

	// Superlative, up to one
	ErrorOr<Superlative> SuperlativeRandom();
	ErrorOr<Superlative> ClosestToActors();

	// Locations
	// Point
	ErrorOr<Point> PositionOf(UnitGroup group);
	// Point MousePosition();


	// This is used to construct the base numbers
	// that take a left param & implicit right param
	ErrorOr<Number> AppendDecimalDigit(Number so_far, Number next);


}; // struct CommandContext;

} // namespace Command

#endif // BRUSHLINK_COMMAND_CONTEXT_H