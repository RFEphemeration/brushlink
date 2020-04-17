#include "CommandContext.h"

namespace Command
{

using ElementType;
using OccurrenceFlags;

void CommandContext::InitElementDictionary()
{
	element_dictionary.clear();

	element_dictionary.insert({
		{"Command", EmptyCommandElement(
			Command,
			{
				OneOf(
				{
					Param(Action)
				})
			}
		)},
		{"Select", MakeContextAction(
			Action,
			CommandContext::Select,
			{
				Param(Selector, "SelectorFriendly")
			}
		)},
		{"Move", MakeContextAction(
			Action,
			CommandContext::Move,
			{
				Param(Selector, "SelectorActors"),
				Param(Location)
			}
		)},
		{"Attack", MakeContextAction(
			ElementType.Action,
			CommandContext::Move,
			{
				Param(Selector, "SelectorActors"),
				Param(Selector, "SelectorTarget")
			}
		)},
		{"SetCommandGroup", MakeContextAction(
			ElementType.Action,
			CommandContext::SetCommandGroup,
			{
				Param(Selector, "SelectorActors")
			}
		)},
		{"Selector", new SelectorCommandElement{}},
		// is this an appropriate way to do context sensitive defaults?
		// it doesn't feel great
		// maybe we should be using child command contexts
		{"SelectorActors", new SelectorCommandElement{
			Param(ElementType.Set, "CurrentSelection"),
			Param(ElementType.Filter, Repeatable | Optional),
			Param(ElementType.GroupSize, Optional),
			Param(ElementType.Superlative, "SuperlativeRandom")
		}},
		{"SelectorFriendly", new SelectorCommandElement{
			Param(ElementType.Set, "Allies"),
			Param(ElementType.Filter, Repeatable | Optional),
			Param(ElementType.GroupSize, Optional),
			Param(ElementType.Superlative, "SuperlativeRandom")
		}},
		{"SelectorTarget", new SelectorCommandElement{
			Param(ElementType.Set, "Enemies"),
			Param(ElementType.Filter, Repeatable | Optional),
			Param(ElementType.GroupSize, Optional),
			Param(ElementType.Superlative, "SuperlativeRandom")
		}},
		{"Enemies", MakeContextFunction(
			ElementType.Set,
			CommandContext::Enemies,
			{}
		)},
		{"Allies", MakeContextFunction(
			ElementType.Set,
			CommandContext::Allies,
			{}
		)},
		{"CurrentSelection", MakeContextFunction(
			ElementType.Set,
			CommandContext::CurrentSelection,
			{}
		)},
		{"Actors", MakeContextFunction(
			ElementType.Set,
			CommandContext::Actors,
			{}
		)},
		{"CommandGroup", MakeContextFunction(
			ElementType.Set,
			CommandContext::CommandGroup,
			{
				Param(Number)
			}
		)},
		{"WithinActorsRange", MakeContextFunction(
			ElementType.Filter,
			CommandContext::WithinActorsRange,
			{
				Param(Number)
			}
		)},
		{"OnScreen", MakeContextFunction(
			ElementType.Filter,
			CommandContext::OnScreen,
			{ }
		)},
		{"GroupSizeLiteral", MakeContextFunction(
			ElementType.GroupSize,
			CommandContext::GroupSizeLiteral,
			{
				Param(Number)
			}
		)},
		{"GroupActorsRatio", MakeContextFunction(
			ElementType.GroupSize,
			CommandContext::GroupActorsRatio,
			{
				Param(Number)
			}
		)},
		{"SuperlativeRandom", MakeContextFunction(
			ElementType.Superlative,
			CommandContext::SuperlativeRandom,
			{ }
		)},
		{"ClosestToActors", MakeContextFunction(
			ElementType.Superlative,
			CommandContext::ClosestToActors,
			{ }
		)},
		{"PositionOf", MakeContextFunction(
			ElementType.Location,
			CommandContext::PositionOf,
			{
				// @Incomplete: the default for this selector
				Param(Selector)
			}
		)},
		/*
		{"MouseInputPosition", MakeContextFunction(
			ElementType.Location,
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

ErrorOr<std::unique_ptr<CommandElement> > CommandContext::GetNewCommandElement(HString name)
{
	if (element_dictionary.contains(name))
	{
		auto copy = element_dictionary[name].DeepCopy();
		// @Incomplete, name should be passed through constructor
		copy->name = name;
		return copy;
	}
	else
	{
		return Error("Couldn't find element with name" + name);
	}
}

ErrorOr<ElementToken> CommandContext::GetTokenForName(ElementName name)
{
	if (element_dictionary.contains(name))
	{
		return ElementToken{element_dictionary[name]->GetType(), name};
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
		if (pair.value > max_type_count)
		{
			max_type_count = pair.value;
		}
	}
	allowed_next_elements[ElementType.Skip] = max_type_count - 1;
	if (command->ParametersSatisfied())
	{
		allowed_next_elements[ElementType.Termination] = 1;
	}
	allowed_next_elements[ElementType.Cancel] = 1;
	return Success();
}


ErrorOr<Success> CommandContext::HandleToken(ElementToken token)
{
	if (!allowed_next_elements.contains(token.type)
		|| allowed_next_elements[token.type] <= 0)
	{
		return Error("This token type is not allowed");
	}
	switch(token.type)
	{
		case ElementType.Skip:
			for (auto pair : allowed_next_elements)
			{
				// we probably need to make this an explicit list?
				if (pair.first == ElementType.Termination
					|| pair.first == ElementType.Cancel)
				{
					continue;
				}
				if (allowed_next_elements[pair.first] <= 1)
				{
					// erase only invalidates the current element's iterator
					// and no other
					allowed_next_elements.erase(pair.first)
				}
				else
				{
					allowed_next_elements[pair.first] -= 1;
				}
			}
			skip_count += 1;
			return Success();
		case ElementType.Termination:
			// @Incomplete what should we do if we can't terminate?
			CHECK_RETURN(command->Evaluate());
			// intentional fallthrough to cancel
		case ElementType.Cancel:
			command = CHECK_RETURN(GetNewCommandElement("Command"));
			break;
		default:
			auto next = CHECK_RETURN(GetNewCommandElement(token.name));
			bool success = CHECK_RETURN(command->AppendArgument(next));
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
void CommandContext::Select(UnitGroup units)
{
	current_selection = units;
}

void CommandContext::Move(UnitGroup actors, Location target)
{
	// @Incomplete implement
	std::cout << "Move " << actors.members[0];
}

void CommandContext::Attack(UnitGroup actors, UnitGroup target)
{
	// @Incomplete implement
	std::cout << "Attack " << actors.members[0] << " " << target.members[0];
}
void CommandContext::SetCommandGroup(UnitGroup actors, Number group)
{
	command_groups[group.value] = actors;
}
// void AddToCommandGroup(UnitGroup actors, Number group);
// void RemoveFromCommandGroup(UnitGroup actors, Number group);

// Set
UnitGroup CommandContext::Enemies()
{
	// @Incomplete implement
	return UnitGroup{{
		0, 2, 4, 6, 8, 10, 12
	}};
}
UnitGroup CommandContext::Allies()
{
	// @Incomplete implement
	return UnitGroup{{
		1, 3, 5, 7, 9, 11, 13
	}};
}
UnitGroup CommandContext::CurrentSelection()
{
	return current_selection;
}
UnitGroup CommandContext::Actors()
{
	return actors_stack[actors_stack.size() - 1];
}
UnitGroup CommandContext::CommandGroup(Number group)
{
	if (command_groups.contains(group.value))
	{
		return command_groups[group.value];
	}
	return UnitGroup{};
}

// Filter, can have many
Filter CommandContext::FilterIdentity()
{
	return [](UnitGroup set) -> UnitGroup { return set; };
}
Filter CommandContext::WithinActorsRange(Number distance_modifier)
{
	std::cout << "WithinActorsRange " << distance_modifier.value << std::endl;
	// @Incomplete implement
	return [distance_modifier](UnitGroup set) -> UnitGroup { return set; };
}
Filter CommandContext::OnScreen()
{
	// @Incomplete implement
	return [](UnitGroup set) -> UnitGroup
	{
		UnitGroup on_screen{};

		for (auto id : set.members)
		{
			if (id <= 5)
			{
				on_screen.members.push_back(id);
			}
		}
		return on_screen;
	};
}

// Group_Size, up to one
GroupSize CommandContext::GroupSizeIdentity()
{
	return [](UnitGroup set) -> Number { return set.members.size(); };
}

GroupSize CommandContext::GroupSizeLiteral(Number size)
{
	return [size](UnitGroup set) -> Number
	{
		return std::min(set.members.size(), size);
	};
}
// Could probably implement this as a word, SizeOf Actors DividedBy ratio
// is UnitGroup even the right return type for GroupSize?
// maybe it should just be a number...
GroupSize CommandContext::GroupActorsRatio(UnitGroup set, Number ratio) // implied 1/
{
	int size = Actors.members.size() / ratio.value;
	if (size == 0)
	{
		size = 1;
	}
	return GroupSize(set, size);
}

// Superlative, up to one
Superlative SuperlativeRandom()
{
	return [](UnitGroup set, Number size)
	{
		if (set.members.size() <= size)
		{
			return set;
		}
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(set.members.begin(), set.members.end(), g);

		set.members.resize(size);
		return set;
	};
}

Superlative CommandContext::ClosestToActors()
{
	return [](UnitGroup set, Number size)
	{
		if (set.members.size() <= size)
		{
			return set;
		}
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(set.members.begin(), set.members.end(), g);

		set.members.resize(size);
		return set;
	};
}

// Location
Location CommandContext::PositionOf(UnitGroup group, Number size);

} // namespace Command
