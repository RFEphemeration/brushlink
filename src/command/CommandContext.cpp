#include "CommandContext.h"

namespace Command
{

using ElementType;

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
				Param(Selector)
			}
		)},
		{"Move", MakeContextAction(
			Action,
			CommandContext::Move,
			{
				Param(Selector),
				Param(Location)
			}
		)},
		{"Attack", MakeContextAction(
			ElementType.Action,
			CommandContext::Move,
			{
				Param(Selector),
				Param(Selector)
			}
		)},
		{"SetCommandGroup", MakeContextAction(
			ElementType.Action,
			CommandContext::SetCommandGroup,
			{
				Param(Selector)
			}
		)},
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
			{}
		)},
		{"GroupSize", MakeContextFunction(
			ElementType.GroupSize,
			CommandContext::GroupSize,
			{
				Param(Number)
			}
		)},
		{"GroupRatio", MakeContextFunction(
			ElementType.GroupSize,
			CommandContext::GroupRatio,
			{
				Param(Number)
			}
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
		return element_dictionary[name].DeepCopy();
	}
	else
	{
		return Error("Couldn't find element with name" + name.ToCString())
	}
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

void CommandContext::Attack(UnitGroup actors, UnitGroup target);
void CommandContext::SetCommandGroup(UnitGroup actors, Number group);
// void AddToCommandGroup(UnitGroup actors, Number group);
// void RemoveFromCommandGroup(UnitGroup actors, Number group);

// Set
UnitGroup CommandContext::Enemies();
UnitGroup CommandContext::Allies();
UnitGroup CommandContext::CurrentSelection();
UnitGroup CommandContext::Actors();
UnitGroup CommandContext::CommandGroup(Number group);

// Filter, can have many
UnitGroup CommandContext::WithinActorsRange(UnitGroup set, Number distance_modifier);
UnitGroup CommandContext::OnScreen(UnitGroup set);

// Group_Size, up to one
UnitGroup CommandContext::GroupSize(UnitGroup set, Number size);
UnitGroup CommandContext::GroupRatio(UnitGroup set, Number ratio); // implied 1/

// Superlative, up to one
UnitGroup CommandContext::ClosestToActors(UnitGroup set);

// Location
Location CommandContext::PositionOf(UnitGroup group);

} // namespace Command
