#include "CommandContext.h"
#include "CommandParameter.hpp"
#include "CommandElement.hpp"

namespace Command
{

namespace ET = ElementType;
using namespace OccurrenceFlags;

void CommandContext::InitElementDictionary()
{
	element_dictionary.clear();

	element_dictionary.insert({
		{"Command", new EmptyCommandElement{
			ET::Command,
			{
				Param(ET::Action)
				/* @Incomplete OneOf doesn't seem to work
				new OneOf(
				{
					Param(ET::Action)
				})
				*/
			}
		}},
		{"Select", MakeContextAction(
			ET::Action,
			&CommandContext::Select,
			{
				Param(ET::Selector, "SelectorFriendly")
			}
		)},
		{"Move", MakeContextAction(
			ET::Action,
			&CommandContext::Move,
			{
				Param(ET::Selector, "SelectorActors"),
				Param(ET::Location)
			}
		)},
		{"Attack", MakeContextAction(
			ET::Action,
			&CommandContext::Move,
			{
				Param(ET::Selector, "SelectorActors"),
				Param(ET::Selector, "SelectorTarget")
			}
		)},
		{"SetCommandGroup", MakeContextAction(
			ET::Action,
			&CommandContext::SetCommandGroup,
			{
				Param(ET::Selector, "SelectorActors")
			}
		)},
		{"Selector", new SelectorCommandElement{{
			Param(ET::Set, Optional),
			Param(ET::Filter, Repeatable | Optional),
			Param(ET::Group_Size, Optional),
			Param(ET::Superlative, Optional)
		}}},
		// is this an appropriate way to do context sensitive defaults?
		// it doesn't feel great
		// maybe we should be using child command contexts
		{"SelectorActors", new SelectorCommandElement{{
			Param(ET::Set, "CurrentSelection"),
			Param(ET::Filter, Repeatable | Optional),
			Param(ET::Group_Size, Optional),
			Param(ET::Superlative, "SuperlativeRandom")
		}}},
		{"SelectorFriendly", new SelectorCommandElement{{
			Param(ET::Set, "Allies"),
			Param(ET::Filter, Repeatable | Optional),
			Param(ET::Group_Size, Optional),
			Param(ET::Superlative, "SuperlativeRandom")
		}}},
		{"SelectorTarget", new SelectorCommandElement{{
			Param(ET::Set, "Enemies"),
			Param(ET::Filter, Repeatable | Optional),
			Param(ET::Group_Size, Optional),
			Param(ET::Superlative, "SuperlativeRandom")
		}}},
		{"Enemies", MakeContextFunction(
			ET::Set,
			&CommandContext::Enemies,
			{}
		)},
		{"Allies", MakeContextFunction(
			ET::Set,
			&CommandContext::Allies,
			{}
		)},
		{"CurrentSelection", MakeContextFunction(
			ET::Set,
			&CommandContext::CurrentSelection,
			{}
		)},
		{"Actors", MakeContextFunction(
			ET::Set,
			&CommandContext::Actors,
			{}
		)},
		{"CommandGroup", MakeContextFunction(
			ET::Set,
			&CommandContext::CommandGroup,
			{
				Param(ET::Number)
			}
		)},
		{"WithinActorsRange", MakeContextFunction(
			ET::Filter,
			&CommandContext::WithinActorsRange,
			{
				Param(ET::Number)
			}
		)},
		{"OnScreen", MakeContextFunction(
			ET::Filter,
			&CommandContext::OnScreen,
			{ }
		)},
		{"GroupSizeLiteral", MakeContextFunction(
			ET::Group_Size,
			&CommandContext::GroupSizeLiteral,
			{
				Param(ET::Number)
			}
		)},
		{"GroupActorsRatio", MakeContextFunction(
			ET::Group_Size,
			&CommandContext::GroupActorsRatio,
			{
				Param(ET::Number)
			}
		)},
		{"SuperlativeRandom", MakeContextFunction(
			ET::Superlative,
			&CommandContext::SuperlativeRandom,
			{ }
		)},
		{"ClosestToActors", MakeContextFunction(
			ET::Superlative,
			&CommandContext::ClosestToActors,
			{ }
		)},
		{"PositionOf", MakeContextFunction(
			ET::Location,
			&CommandContext::PositionOf,
			{
				// @Incomplete: the default for this selector
				Param(ET::Selector)
			}
		)},
		/*
		{"MouseInputPosition", MakeContextFunction(
			ET::Location,
			CommandContext::MouseInputPosition,
			{ }
		)},
		*/
		// maybe numbers shouldn't be literals because of left parameter for *10
		{"Zero", MakeLiteral(Number(0))},
		{"One", MakeLiteral(Number(1))},
		{"Two", MakeLiteral(Number(2))},
		{"Three", MakeLiteral(Number(3))},
		{"Four", MakeLiteral(Number(4))},
		{"Five", MakeLiteral(Number(5))},
		{"Six", MakeLiteral(Number(6))},
		{"Seven", MakeLiteral(Number(7))},
		{"Eight", MakeLiteral(Number(8))},
		{"Nine", MakeLiteral(Number(9))}
	});

	// todo: load user defined words from save
}

ErrorOr<value_ptr<CommandElement> > CommandContext::GetNewCommandElement(HString name)
{
	if (element_dictionary.find(name) != element_dictionary.end())
	{
		auto copy = element_dictionary[name]->clone();
		// @Incomplete, name should be passed through constructor
		copy->name = name;
		return value_ptr<CommandElement>(copy);
	}
	else
	{
		return Error("Couldn't find element with name" + name);
	}
}

ErrorOr<ElementToken> CommandContext::GetTokenForName(ElementName name)
{
	if (element_dictionary.find(name.value) != element_dictionary.end())
	{
		return ElementToken{element_dictionary[name.value]->Type(), name};
	}
	else
	{
		return Error("Couldn't find element with name" + name.value);
	}
}

ErrorOr<Success> CommandContext::InitNewCommand()
{
	command = CHECK_RETURN(GetNewCommandElement("Command"));
	return RefreshAllowedTypes();
}


ErrorOr<Success> CommandContext::RefreshAllowedTypes()
{
	allowed_next_elements = command->GetAllowedArgumentTypes();
	int max_type_count = 1;
	for (auto pair : allowed_next_elements)
	{
		if (pair.second > max_type_count)
		{
			max_type_count = pair.second;
		}
	}
	allowed_next_elements[ET::Skip] = max_type_count - 1;
	if (command->ParametersSatisfied())
	{
		allowed_next_elements[ET::Termination] = 1;
	}
	allowed_next_elements[ET::Cancel] = 1;
	return Success();
}


ErrorOr<Success> CommandContext::GetAllowedNextElements(Set<ElementName> & allowed)
{
	allowed.clear();
	for (auto & pair : element_dictionary)
	{
		auto type = pair.second->Type();
		if (allowed_next_elements.count(type) > 0
			&& allowed_next_elements[type] > 0)
		{
			allowed.insert(pair.first);
		}
	}
	return Success();
}


ErrorOr<Success> CommandContext::HandleToken(ElementToken token)
{
	if (allowed_next_elements.count(token.type) == 0
		|| allowed_next_elements[token.type] <= 0)
	{
		return Error("This token type is not allowed");
	}
	switch(token.type)
	{
		case ET::Skip:
			for (auto pair : allowed_next_elements)
			{
				// we probably need to make this an explicit list?
				if (pair.first == ET::Termination
					|| pair.first == ET::Cancel)
				{
					continue;
				}
				if (allowed_next_elements[pair.first] <= 1)
				{
					// erase only invalidates the current element's iterator
					// and no other
					allowed_next_elements.erase(pair.first);
				}
				else
				{
					allowed_next_elements[pair.first] -= 1;
				}
			}
			skip_count += 1;
			return Success();
		case ET::Termination:
			// @Incomplete what should we do if we can't terminate?
			CHECK_RETURN(command->Evaluate(*this));
			// intentional fallthrough to cancel
		case ET::Cancel:
			command = CHECK_RETURN(GetNewCommandElement("Command"));
			break;
		default:
			auto next = CHECK_RETURN(GetNewCommandElement(token.name.value));
			int skips = skip_count; // copying here, append takes a ref and modifies
			bool success = CHECK_RETURN(command->AppendArgument(std::move(next), skips));
			if (!success)
			{
				return Error("Couldn't append argument even though it was supposed to be okay. This shouldn't happen");
			}
			break;
	}

	return RefreshAllowedTypes();
}


void CommandContext::PushActors(UnitGroup group)
{
	actors_stack.push_back(group);
}

void CommandContext::PopActors()
{
	actors_stack.pop_back();
}

//Action
ErrorOr<Success> CommandContext::Select(UnitGroup units)
{
	current_selection = units;
	return Success();
}

ErrorOr<Success> CommandContext::Move(UnitGroup actors, Location target)
{
	// @Incomplete implement
	std::cout << "Move " << actors.members[0].value;
	return Success();
}

ErrorOr<Success> CommandContext::Attack(UnitGroup actors, UnitGroup target)
{
	// @Incomplete implement
	std::cout << "Attack " << actors.members[0].value << " " << target.members[0].value;
	return Success();
}
ErrorOr<Success> CommandContext::SetCommandGroup(UnitGroup actors, Number group)
{
	command_groups[group.value] = actors;
	return Success();
}
// void AddToCommandGroup(UnitGroup actors, Number group);
// void RemoveFromCommandGroup(UnitGroup actors, Number group);

// Set
ErrorOr<UnitGroup> CommandContext::Enemies()
{
	// @Incomplete implement
	return UnitGroup{{
		0, 2, 4, 6, 8, 10, 12
	}};
}
ErrorOr<UnitGroup> CommandContext::Allies()
{
	// @Incomplete implement
	return UnitGroup{{
		1, 3, 5, 7, 9, 11, 13
	}};
}
ErrorOr<UnitGroup> CommandContext::CurrentSelection()
{
	return current_selection;
}
ErrorOr<UnitGroup> CommandContext::Actors()
{
	return actors_stack.back();
}
ErrorOr<UnitGroup> CommandContext::CommandGroup(Number group)
{
	if (command_groups.count(group.value) > 0)
	{
		return command_groups[group.value];
	}
	return UnitGroup{};
}

// Filter, can have many
ErrorOr<Filter> CommandContext::WithinActorsRange(Number distance_modifier)
{
	std::cout << "WithinActorsRange " << distance_modifier.value << std::endl;
	// @Incomplete implement
	return Filter{
		[distance_modifier](UnitGroup set) -> UnitGroup
		{
			return set;
		}
	};
}
ErrorOr<Filter> CommandContext::OnScreen()
{
	// @Incomplete implement
	return Filter{
		[](UnitGroup set) -> UnitGroup
		{
			UnitGroup on_screen{};

			for (auto id : set.members)
			{
				if (id.value <= 5)
				{
					on_screen.members.push_back(id);
				}
			}
			return on_screen;
		}
	};
}

// Group_Size, up to one
ErrorOr<GroupSize> CommandContext::GroupSizeLiteral(Number size)
{
	return GroupSize{
		[size](UnitGroup set) -> Number
		{
			return std::min(static_cast<int>(set.members.size()), size.value);
		}
	};
}
// Could probably implement this as a word, SizeOf Actors DividedBy ratio
// is UnitGroup even the right return type for GroupSize?
// maybe it should just be a number...
ErrorOr<GroupSize> CommandContext::GroupActorsRatio(Number ratio) // implied 1/
{
	int size = CHECK_RETURN(Actors()).members.size() / ratio.value;
	if (size == 0)
	{
		size = 1;
	}
	return GroupSizeLiteral(size);
}

// Superlative, up to one
ErrorOr<Superlative> CommandContext::SuperlativeRandom()
{
	return Superlative{
		[](UnitGroup set, Number size) -> UnitGroup
		{
			if (set.members.size() <= size.value)
			{
				return set;
			}
			std::random_device rd;
			std::mt19937 g(rd());
			std::shuffle(set.members.begin(), set.members.end(), g);

			set.members.resize(size.value);
			return set;
		}
	};
}

ErrorOr<Superlative> CommandContext::ClosestToActors()
{
	return Superlative{
		[](UnitGroup set, Number size) -> UnitGroup
		{
			if (set.members.size() <= size.value)
			{
				return set;
			}
			std::random_device rd;
			std::mt19937 g(rd());
			std::shuffle(set.members.begin(), set.members.end(), g);

			set.members.resize(size.value);
			return set;
		}
	};
}

// Location
ErrorOr<Location> CommandContext::PositionOf(UnitGroup group)
{
	// @Incomplete actual unit positions
	Point total{0,0};
	if (group.members.size() == 0)
	{
		return Location{total};
	}
	for (auto unit : group.members)
	{
		total = total + Point(unit.value, unit.value);
	}
	return Location{total / group.members.size()};
}

} // namespace Command
