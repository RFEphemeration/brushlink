#include "CommandContext.h"

#include "ContainerExtensions.hpp"
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
	//{ET::Set, {ET::Number}} // Command Group, this feels like it would come up unintentionally too often
	{ET::Number, {ET::Digit}} // Number literal
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
				case ET::Number:
					name = "NumberLiteral";
				default:
					Error("Add more element types to the implied elements constructor").Log();
			}
			implied_elements[next_type][implied_type] = name;
		}
	}
	return implied_elements;
}();

const Set<ElementType::Enum> CommandContext::instruction_element_types{
	ET::Skip, ET::Undo, ET::Redo, ET::Termination, ET::Cancel
};

/*

void CommandContext::AddDef(CommandElement * element)
{
	element_dictionary.insert({element->name, element});
}

using json = nlohmann::json;

template<typename T>
ErrorOr<T> GetAs(json j, std::string member)
{
	if (!j.contains(member))
	{
		return Error("object does not contain " + member);
	}

}

template<typename T>
T GetOrDefault(json j, std::string member, T value)
{
	auto result = GetAs<T>(j, member);
	if (result.IsError())
	{
		return value;
	}
	return result.GetValue();
}

ErrorOr<value_ptr<CommandElement>> GetDef(json j)
{
	if (j.type() != json::value_t::object)
	{
		return Error("Expected definition to be an object");
	}
	std::string name = CHECK_RETURN(GetAs<std::string>(j, "name"));
}

*/

void CommandContext::InitElementDictionary()
{

	auto WithImplied = [&](
		value_ptr<CommandElement>& element,
		int param_index,
		value_ptr<CommandElement>&& implied_value) -> value_ptr<CommandElement>&
	{
		implied_value->implicit = Implicit::Child;
		element->parameters[param_index]->RemoveLastArgument();
		auto result = element->parameters[param_index]->SetArgument(*this, std::move(implied_value));
		if (result.IsError())
		{
			result.GetError().Log();
		}
		return element;
	};

	element_dictionary.clear();

	// basic control elements
	element_dictionary.insert({
		{"Termination", new EmptyCommandElement{
			{"Termination", ET::Termination}
		}},
		{"Cancel", new EmptyCommandElement{
			{"Cancel", ET::Cancel}
		}},
		{"Skip", new EmptyCommandElement{
			{"Skip", ET::Skip}
		}},
		{"Undo", new EmptyCommandElement{
			{"Undo", ET::Undo}
		}},
		{"Redo", new EmptyCommandElement{
			{"Redo", ET::Redo}
		}}
	});

	// elements that are hidden or used in implied params
	private_element_dictionary.insert({
		{"Selector", new SelectorCommandElement{
			new ParamSingleRequired{ET::Set}
		}},
		{"SelectorActors", new SelectorCommandElement{
			new ParamSingleOptional{ET::Set, "CurrentSelection"}
		}},
		{"SelectorFriendly", new SelectorCommandElement{
			new ParamSingleOptional{ET::Set, "Allies"}
		}},
		{"SelectorTarget", new SelectorCommandElement{
			new ParamSingleOptional{ET::Set, "Enemies"}
		}},
		{"Location", new ContextFunction{
			{"Location", ET::Location, {
				new OneOf({
					Param(*this, ET::Point),
					Param(*this, ET::Line),
					Param(*this, ET::Direction),
					Param(*this, ET::Area)
				})
			}},
			&CommandContext::LocationConversion
		}},
		{"NumberLiteral", new NumberLiteralCommandElement{}}
	});

	// in stages...
	element_dictionary.insert({
		{"Select", new ContextFunctionWithActors{
			{"Select", ET::Action, {
				ParamImplied(*this, "SelectorFriendly")
			}},
			&CommandContext::Select
		}},
		{"CommandGroup", new ContextFunction{
			{"CommandGroup", ET::Set, {
				new ParamSingleRequired(ET::Number)
			}},
			&CommandContext::CommandGroup
		}},
	});

/*
	private_element_dictionary.insert({
		{"Selector/CommandGroup", }
	});

	private_element_dictionary.insert({
		{"Select/Selector/CommandGroup", new ContextFunctionWithActors{
			{"Select", ET::Action, {
				ParamImplied(*this, GetNewCommandElement("Selector/CommandGroup").GetValue())
			}},
			&CommandContext::Select
		}},
	});
	*/

	element_dictionary.insert({
		{"Command", new EmptyCommandElement{{
			"Command", ET::Command, {
				// @Bug load order is too delicate and annoying
				// consider loading from text in passes
				new ParamSingleImpliedOptions(ET::Action, std::vector<value_ptr<CommandElement>>{
					GetNewCommandElement("Select").GetValue(),
					WithImplied(
						GetNewCommandElement("Select").GetValue(),
						0,
						new SelectorCommandElement{
							ParamImplied(*this, new ContextFunction{
								{"CommandGroup", ET::Set, {
									new ParamSingleRequired(ET::Number)
								}},
								&CommandContext::CommandGroup
							})
						}
					)
				})
				// Param(*this, ET::Action)
				/* @Incomplete OneOf doesn't seem to work
				new OneOf(
				{
					Param(ET::Action)
				})
				*/
			}, Implicit::Child // for Undo purposes, Command is always present
		}}}
	});

	// the rest of the non-word elements
	element_dictionary.insert({
		{"Move", new ContextFunctionWithActors{
			{"Move", ET::Action, {
				ParamImplied(*this, "SelectorActors"),
				ParamImplied(*this, "Location")
			}},
			&CommandContext::Move,
		}},
		{"Attack", new ContextFunctionWithActors{
			{"Attack", ET::Action, {
				ParamImplied(*this, "SelectorActors"),
				ParamImplied(*this, "SelectorTarget")
			}},
			&CommandContext::Attack,
		}},
		{"SetCommandGroup", new ContextFunctionWithActors{
			{"SetCommandGroup", ET::Action, {
				ParamImplied(*this, "SelectorActors"),
				new ParamSingleRequired(ET::Number)
			}},
			&CommandContext::SetCommandGroup,
		}},
		{"Enemies", new ContextFunction{
			{ "Enemies", ET::Set, { } },
			&CommandContext::Enemies,
		}},
		{"Allies", new ContextFunction{
			{ "Allies", ET::Set, { } },
			&CommandContext::Allies,
		}},
		{"CurrentSelection", new ContextFunction{
			{ "CurrentSelection", ET::Set, { } },
			&CommandContext::CurrentSelection,
		}},
		{"Actors", new ContextFunction{
			{ "Actors", ET::Set, { } },
			&CommandContext::Actors,
		}},
		{"WithinActorsRange", new ContextFunction{
			{ "WithinActorsRange", ET::Filter, { 
				new ParamSingleRequired(ET::Number)
			}},
			&CommandContext::WithinActorsRange,
		}},
		{"OnScreen", new ContextFunction{
			{ "OnScreen", ET::Filter, { }},
			&CommandContext::OnScreen,
		}},
		{"GroupSizeLiteral", new ContextFunction{
			{ "GroupSizeLiteral", ET::Group_Size, {
				new ParamSingleRequired(ET::Number)
			}},
			&CommandContext::GroupSizeLiteral,
		}},
		{"GroupActorsRatio", new ContextFunction{
			{ "GroupActorsRatio", ET::Group_Size, {
				new ParamSingleRequired(ET::Number)
			}},
			&CommandContext::GroupActorsRatio,
		}},
		{"SuperlativeRandom", new ContextFunction{
			{ "SuperlativeRandom", ET::Superlative, { }},
			&CommandContext::SuperlativeRandom,
		}},
		{"ClosestToActors", new ContextFunction{
			{ "ClosestToActors", ET::Superlative, { }},
			&CommandContext::ClosestToActors,
		}},
		{"ClosestToActors", new ContextFunction{
			{ "ClosestToActors", ET::Superlative, { }},
			&CommandContext::ClosestToActors,
		}},
		{"PositionOf", new ContextFunction{
			{ "PositionOf", ET::Point, {
				ParamImplied(*this, "Selector")
			}},
			&CommandContext::PositionOf,
		}},
		/*
		{"MouseInputPosition", MakeContextFunction(
			ET::Location,
			CommandContext::MouseInputPosition,
			{ }
		)},
		*/
		{"Zero",  new Literal{"Zero",  Digit{0}}},
		{"One",   new Literal{"One",   Digit{1}}},
		{"Two",   new Literal{"Two",   Digit{2}}},
		{"Three", new Literal{"Three", Digit{3}}},
		{"Four",  new Literal{"Four",  Digit{4}}},
		{"Five",  new Literal{"Five",  Digit{5}}},
		{"Six",   new Literal{"Six",   Digit{6}}},
		{"Seven", new Literal{"Seven", Digit{7}}},
		{"Eight", new Literal{"Eight", Digit{8}}},
		{"Nine",  new Literal{"Nine",  Digit{9}}},
	});

	/*
	auto add_number = [&](std::string name, int value)
	{
		element_dictionary.insert({
			{name, MakeContextFunction(
				ET::Number,
				&CommandContext::AppendDecimalDigit,
				// left param of implied literal Zero, is the name okay?
				// could use the digit "0"
				// an implied options with children that don't have parameters
				// behaves just like a default value
				new ParamSingleImpliedOptions(ET::Number, {
					MakeLiteral(Number(0), "Zero")
				}),
				{
					// can we re-use the name for the literal or is that confusing?
					ParamImplied(*this, name, MakeLiteral(Number(value)))
				}
			)},
		});
	};
	add_number("Zero",  0);
	add_number("One",   1);
	add_number("Two",   2);
	add_number("Three", 3);
	add_number("Four",  4);
	add_number("Five",  5);
	add_number("Six",   6);
	add_number("Seven", 7);
	add_number("Eight", 8);
	add_number("Nine",  9);
	*/

	/* // word elements defined in terms of the others
	element_dictionary.insert({

	});
	*/

	// todo: load user defined words from save
}

ErrorOr<value_ptr<CommandElement> > CommandContext::GetNewCommandElement(HString name)
{
	if (Contains(element_dictionary, name))
	{
		return value_ptr<CommandElement>(element_dictionary[name]->clone());
	}
	else if (Contains(private_element_dictionary, name))
	{
		return value_ptr<CommandElement>(private_element_dictionary[name]->clone());
	}
	else
	{
		return Error("Couldn't find element with name " + name);
	}
}

ErrorOr<ElementToken> CommandContext::GetTokenForName(ElementName name)
{
	if (Contains(element_dictionary, name.value))
	{
		return ElementToken{
			element_dictionary[name.value]->Type(),
			name,
			element_dictionary[name.value]->left_parameter != nullptr
		};
	}
	else if (Contains(private_element_dictionary, name.value))
	{
		return ElementToken{
			private_element_dictionary[name.value]->Type(),
			name,
			private_element_dictionary[name.value]->left_parameter != nullptr
		};
	}
	else
	{
		return Error("Couldn't find element with name " + name.value);
	}
}

std::vector<ElementToken> CommandContext::GetAllTokens()
{
	std::vector<ElementToken> tokens;
	for (auto&& pair : element_dictionary)
	{
		tokens.emplace_back(
			pair.second->Type(),
			pair.first,
			pair.second->left_parameter != nullptr
		);
	}
	return tokens;
}

ErrorOr<Success> CommandContext::InitNewCommand()
{
	command = CHECK_RETURN(GetNewCommandElement("Command"));
	skip_count = 0;
	undo_count = 0;
	undo_stack.clear();
	if (actors_stack.size() != 0)
	{
		Error("Initing new command and actors stack isn't empty").Log();
		actors_stack.clear();
	}
	RefreshAllowedTypes();
	return Success();
}


void CommandContext::RefreshAllowedTypes()
{
	allowed.priority.clear();
	allowed.total_right.clear();
	allowed.total_left.clear();
	command->GetAllowedArgumentTypes(allowed);

	Table<ElementType::Enum, int> allowed_combined;
	int max_type_count = 1;
	for (auto&& pair : allowed.total_left)
	{
		allowed_combined[pair.first] = pair.second;
		if (pair.second > max_type_count)
		{
			max_type_count = pair.second;
		}
	}
	for (auto&& pair : allowed.total_right)
	{
		allowed_combined[pair.first] += pair.second;

		if (allowed_combined[pair.first] > max_type_count)
		{
			max_type_count = allowed_combined[pair.first];
		}
	}

	allowed.total_instruction[ET::Skip] = max_type_count - 1;
	allowed.total_instruction[ET::Undo] = undo_stack.size() - undo_count;
	allowed.total_instruction[ET::Redo] = undo_count;

	if (command->ParametersSatisfied())
	{
		allowed.total_instruction[ET::Termination] = 1;
	}
	else
	{
		allowed.total_instruction[ET::Termination] = 0;
	}
	allowed.total_instruction[ET::Cancel] = 1;
}

ErrorOr<Success> CommandContext::PerformUndo()
{
	// we just performed an Undo, so we can do one fewer
	allowed.total_instruction[ET::Undo] -= 1;
	allowed.total_instruction[ET::Redo] += 1;
	undo_count += 1;
	if (skip_count == 0)
	{
		// undoing an append
		bool remove_command = CHECK_RETURN(command->RemoveLastExplicitElement());
		if (remove_command)
		{
			return Error("We shouldn't ever need to remove the root command");
		}
		RefreshAllowedTypes();
	}
	else
	{
		// undoing a skip
		skip_count -= 1;
		allowed.total_instruction[ET::Skip] += 1;
	}
	return Success();
}

ErrorOr<Success> CommandContext::PerformRedo()
{
	if (undo_count <= 0 || undo_stack.size() == 0)
	{
		return Error("Nothing to Redo");
	}
	ElementToken token = undo_stack[undo_stack.size() - undo_count];
	undo_count -= 1;

	if (token.type == ET::Skip)
	{
		skip_count += 1;
		allowed.total_instruction[ET::Skip] -= 1;
		allowed.total_instruction[ET::Undo] = undo_stack.size() - undo_count;
		allowed.total_instruction[ET::Redo] = undo_count;
	}
	else
	{
		CHECK_RETURN(AppendElement(
			CHECK_RETURN(GetNewCommandElement(token.name.value))
		));
		RefreshAllowedTypes();
	}
	return Success();
}

void CommandContext::BreakUndoChain(ElementToken token)
{
	if (undo_count > 0)
	{
		// lose the redo stack on new element
		// resize requires a default value to emplace if it gets bigger
		// we don't expect to actually put tokens on the back
		undo_stack.resize(undo_stack.size() - undo_count, token);
	}
	undo_stack.push_back(token);
	undo_count = 0;
	allowed.total_instruction[ET::Undo] = undo_stack.size();
	allowed.total_instruction[ET::Redo] = 0;
}

bool CommandContext::IsAllowed(ElementToken token)
{
	// should we assume tokens have associated elements?
	if (!Contains(element_dictionary, token.name.value)
		&& !Contains(private_element_dictionary, token.name.value))
	{
		return false;
	}

	if (Contains(allowed.total_instruction, token.type)
		&& allowed.total_instruction[token.type] > 0)
	{
		return true;
	}

	int allowed_count = allowed.total_right[token.type];
	if (token.has_left_param)
	{
		allowed_count += allowed.total_left[token.type];
	}
	if (skip_count >= allowed_count)
	{
		return false;
	}

	return true;
}

void CommandContext::GetAllowedNextElements(Set<ElementName> & allowed)
{
	allowed.clear();
	for (auto & pair : element_dictionary)
	{
		ElementToken token{
			pair.second->Type(),
			pair.first,
			pair.second->left_parameter != nullptr
		};
		if (IsAllowed(token))
		{
			allowed.insert(pair.first);
		}
	}
}

ErrorOr<Success> CommandContext::AppendElement(value_ptr<CommandElement>&& next)
{
	int skips = skip_count; // copying here, append takes a ref and modifies
	bool success = CHECK_RETURN(command->AppendArgument(
		*this, std::move(next), skips));
	if (!success)
	{
		return Error("Couldn't append argument even though it was supposed to be okay. This shouldn't happen");
	}
	return Success();
}


ErrorOr<Success> CommandContext::HandleToken(ElementToken token)
{
	if (!IsAllowed(token))
	{
		return Error("This token type is not allowed");
	}
	switch(token.type)
	{
		case ET::Skip:
			skip_count += 1;
			allowed.total_instruction[ET::Skip] -= 1;
			BreakUndoChain(token);
			return Success();
		case ET::Undo:
			return PerformUndo();
		case ET::Redo:
			return PerformRedo();
		case ET::Termination:
			// @Incomplete what should we do if we can't terminate?
			CHECK_RETURN(command->Evaluate(*this));
			return InitNewCommand();
		case ET::Cancel:
			// command = CHECK_RETURN(GetNewCommandElement("Command"));
			return InitNewCommand();
		default:
			CHECK_RETURN(AppendElement(
				CHECK_RETURN(GetNewCommandElement(token.name.value))
			));
			BreakUndoChain(token);
			RefreshAllowedTypes();
			return Success();
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
