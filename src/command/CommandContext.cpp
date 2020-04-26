#include "CommandContext.h"

#include "VariantExtensions.hpp"

#include "CommandParameter.hpp"
#include "CommandElement.hpp"

namespace Command
{

namespace ET = ElementType;
using namespace OccurrenceFlags;

// @Incomplete implied node priority
// implied type -> allowed new element types
const Table<ET::Enum, Set<ET::Enum>> CommandContext::allowed_with_implied{
	//{ET::Selector, {ET::Set, ET::Filter, ET::Group_Size, ET::Superlative}},
	//{ET::Location, {ET::Point, ET::Line, ET::Direction, ET::Area}},
	{ET::Set, {ET::Number}} // Command Group, this feels like it would come up unintentionally too often
};

// @Incomplete implied node priority
// next element type -> implied type -> implied element
// because the same argument type might cause different implications
// depending on context.
// we should probably just type this out to allow that
const Table<ET::Enum, Table<ET::Enum, ElementName>> CommandContext::implied_elements = []{
	Table<ET::Enum, Table<ET::Enum, ElementName>> implied_elements;
	for (auto&& implied_pair : allowed_with_implied)
	{
		ET::Enum implied_type = implied_pair.first;
		for(auto&& next_type : implied_pair.second)
		{
			ElementName name;
			switch(implied_type)
			{
				case ET::Selector:
					name = "Selector";
					break;
				case ET::Location:
					name = "Location";
					break;
				case ET::Set:
					// this might depend on next_type, too
					// in which case we might as well just type them out
					name = "CommandGroup";
				default:
					Error("Add more element types to the implied elements constructor").Log();
			}
			implied_elements[next_type][implied_type] = name;
		}
	}
	return implied_elements;
}();

void CommandContext::InitElementDictionary()
{
	element_dictionary.clear();

	// basic control elements
	element_dictionary.insert({
		{"Command", new EmptyCommandElement{
			ET::Command,
			{
				Param(*this, ET::Action)
				/* @Incomplete OneOf doesn't seem to work
				new OneOf(
				{
					Param(ET::Action)
				})
				*/
			}
		}},
		{"Termination", new EmptyCommandElement{
			ET::Termination, { }
		}},
		{"Cancel", new EmptyCommandElement{
			ET::Cancel, { }
		}},
		{"Skip", new EmptyCommandElement{
			ET::Skip, { }
		}}
		// @Feature Undo/Back
	});

	// elements that are hidden or used in implied params
	element_dictionary.insert({
		{"Selector", new SelectorCommandElement{{
			Param(*this, ET::Set),
			Param(*this, ET::Filter, Repeatable | Optional),
			Param(*this, ET::Group_Size, Optional),
			Param(*this, ET::Superlative, "SuperlativeRandom")
		}}},
		// is this an appropriate way to do context sensitive defaults?
		// it doesn't feel great
		// maybe we should be using child command contexts
		{"SelectorActors", new SelectorCommandElement{{
			Param(*this, ET::Set, "CurrentSelection"),
			Param(*this, ET::Filter, Repeatable | Optional),
			Param(*this, ET::Group_Size, Optional),
			Param(*this, ET::Superlative, "SuperlativeRandom")
		}}},
		{"SelectorFriendly", new SelectorCommandElement{{
			Param(*this, ET::Set, "Allies"),
			Param(*this, ET::Filter, Repeatable | Optional),
			Param(*this, ET::Group_Size, Optional),
			Param(*this, ET::Superlative, "SuperlativeRandom")
		}}},
		{"SelectorTarget", new SelectorCommandElement{{
			Param(*this, ET::Set, "Enemies"),
			Param(*this, ET::Filter, Repeatable | Optional),
			Param(*this, ET::Group_Size, Optional),
			Param(*this, ET::Superlative, "SuperlativeRandom")
		}}},
		{"Location", MakeContextFunction(
			ET::Location,
			&CommandContext::LocationConversion,
			{
				new OneOf(
				{
					Param(*this, ET::Point),
					Param(*this, ET::Line),
					Param(*this, ET::Direction),
					Param(*this, ET::Area)
				})
			}
		)}
	});

	// the rest of the non-word elements
	element_dictionary.insert({
		{"Select", MakeContextAction(
			ET::Action,
			&CommandContext::Select,
			{
				Param(*this, ET::Selector, "SelectorFriendly", Implied)
			}
		)},
		{"Move", MakeContextAction(
			ET::Action,
			&CommandContext::Move,
			{
				Param(*this, ET::Selector, "SelectorActors", Implied),
				// @Bug two implied elements locks off the first
				Param(*this, ET::Location, "Location", Implied)
			}
		)},
		{"Attack", MakeContextAction(
			ET::Action,
			&CommandContext::Move,
			{
				Param(*this, ET::Selector, "SelectorActors", Implied),
				Param(*this, ET::Selector, "SelectorTarget", Implied)
			}
		)},
		{"SetCommandGroup", MakeContextAction(
			ET::Action,
			&CommandContext::SetCommandGroup,
			{
				Param(*this, ET::Selector, "SelectorActors", Implied),
				Param(*this, ET::Number)
			}
		)},
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
				Param(*this, ET::Number)
			}
		)},
		{"WithinActorsRange", MakeContextFunction(
			ET::Filter,
			&CommandContext::WithinActorsRange,
			{
				Param(*this, ET::Number)
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
				Param(*this, ET::Number)
			}
		)},
		{"GroupActorsRatio", MakeContextFunction(
			ET::Group_Size,
			&CommandContext::GroupActorsRatio,
			{
				Param(*this, ET::Number)
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
			ET::Point,
			&CommandContext::PositionOf,
			{
				// @Incomplete: the default for this selector
				Param(*this, ET::Selector, "Selector", Implied)
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

	/* // word elements defined in terms of the others
	element_dictionary.insert({

	});
	*/

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
	skip_count = 0;
	if (actors_stack.size() != 0)
	{
		Error("Initing new command and actors stack isn't empty").Log();
		actors_stack.clear();
	}
	return RefreshAllowedTypes();
}


ErrorOr<Success> CommandContext::RefreshAllowedTypes()
{
	allowed_next_elements = command->GetAllowedArgumentTypes();

	// allowed implied elements are being computed inside CommandElement
	// so that we can ensure only 1 skip count allowed per parameter
	// @Cleanup remove this when convinced it's not needed
	/* 
	Table<ElementType::Enum, int> allowed_next_with_implied;
	for (auto&& pair : allowed_next_elements)
	{
		for (auto&& type : allowed_with_implied[pair.first])
		{
			allowed_next_with_implied[type] += pair.second;
		}
	}

	for(auto&& pair : allowed_next_with_implied)
	{
		allowed_next_elements[pair.first] += pair.second;
	}
	*/

	int max_type_count = 1;
	for (auto&& pair : allowed_next_elements)
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


ErrorOr<Success> CommandContext::DecrementAllowedNextFromSkip()
{
	for (auto& pair : allowed_next_elements)
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
			DecrementAllowedNextFromSkip();
			skip_count += 1;
			return Success();
		case ET::Termination:
			// @Incomplete what should we do if we can't terminate?
			CHECK_RETURN(command->Evaluate(*this));
			// intentional fallthrough to cancel
		case ET::Cancel:
			// command = CHECK_RETURN(GetNewCommandElement("Command"));
			return InitNewCommand();
		default:
			auto next = CHECK_RETURN(GetNewCommandElement(token.name.value));
			int skips = skip_count; // copying here, append takes a ref and modifies
			bool success = CHECK_RETURN(command->AppendArgument(
				*this, std::move(next), skips));
			if (!success)
			{
				return Error("Couldn't append argument even though it was supposed to be okay. This shouldn't happen");
			}
			return RefreshAllowedTypes();
	}
}


void CommandContext::PushActors(UnitGroup group)
{
	actors_stack.push_back(group);
}

void CommandContext::PopActors()
{
	actors_stack.pop_back();
}

std::string CommandContext::ToLogString(Value v)
{
	return std::visit([](auto&& arg) -> std::string
	{
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same<T, Number>::value)
		{
			return std::to_string(arg.value);
		}
		else if constexpr (std::is_same<T, UnitGroup>::value)
		{
			int count = arg.members.size();
			int display_count = std::min(count, 5);
			std::string ret = "";
			if (count > 5)
			{
				ret += std::to_string(count) + " [";
			}
			else
			{
				ret += "[";
			}
			for (int i = 0; i < display_count; i++)
			{
				ret += std::to_string(arg.members[i].value) + ", ";
			}
			if (count > 0 && count <= 5)
			{
				ret.pop_back();
				ret.pop_back();
			}
			else if (count > 0)
			{
				ret += "...";
			}
			ret += "]";
			return ret;
		}
		return "uknown";
	}, v);
}

void CommandContext::LogAction(std::string entry)
{
	action_log.push_back(entry);
	std::cout << entry << std::endl;
}

ErrorOr<Location> CommandContext::LocationConversion(Value value)
{
	if (std::holds_alternative<Point>(value))
	{
		return Location{std::get<Point>(value)};
	}
	else if (std::holds_alternative<Line>(value))
	{
		return Location{std::get<Line>(value)};
	}
	else if (std::holds_alternative<Direction>(value))
	{
		return Location{std::get<Direction>(value)};
	}
	else if (std::holds_alternative<Area>(value))
	{
		return Location{std::get<Area>(value)};
	}
	return Error("Location's value does not hold any of Point, Line, Direction, Area");
}

//Action
ErrorOr<Success> CommandContext::Select(UnitGroup units)
{
	current_selection = units;
	LogAction("Select " + ToLogString(units));
	return Success();
}

ErrorOr<Success> CommandContext::Move(UnitGroup actors, Location target)
{
	// @Incomplete implement
	LogAction("Move " + ToLogString(actors) + " to " + ToLogString(target));
	// std::cout << "Move " << actors.members[0].value << std::endl;
	return Success();
}

ErrorOr<Success> CommandContext::Attack(UnitGroup actors, UnitGroup target)
{
	// @Incomplete implement
	LogAction("Attack with " + ToLogString(actors) + " at " + ToLogString(target));
	return Success();
}
ErrorOr<Success> CommandContext::SetCommandGroup(UnitGroup actors, Number group)
{
	command_groups[group.value] = actors;
	LogAction("SetCommandGroup " + ToLogString(group) + " to " + ToLogString(actors));
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
	// @Behavior falling back to current_selection helps with reliability
	// I think...
	// but if used in the middle of forming actors
	// one might think of it as actors so far, after filters have been applied
	if (actors_stack.size() == 0)
	{
		return current_selection;
	}
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
ErrorOr<Point> CommandContext::PositionOf(UnitGroup group)
{
	// @Incomplete actual unit positions
	Point total{0,0};
	if (group.members.size() == 0)
	{
		return total;
	}
	for (auto unit : group.members)
	{
		total = total + Point(unit.value, unit.value);
	}
	return total / group.members.size();
}

} // namespace Command
