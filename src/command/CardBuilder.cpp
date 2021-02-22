#include "CardBuilder.h"

#include "ContainerExtensions.hpp"
#include "TerminalExtensions.h"

namespace Command
{

const int default_columns = 4;
const int default_rows = 4;
const Table<std::string, CardInput> default_hotkeys{
	{"Tab", {TabNav::NextHighestPriority}},
	{"Back", {TabNav::Back}},
	{"Left", {TabNav::Left}},
	{"Right", {TabNav::Right}},

	// @Feature difference between hold shift and tap shift
	// @Bug modifier key input detection
	// could consider reserving shift for punctuation / tab nav input
	// or perhaps one of the other modifiers, maybe CTRL for commands, ALT for nav?
	{"Shift", {TabNav::MoreOptionsForCurrentType}},
	{"Escape", {Instruction::Cancel}},
	{"Space", {Instruction::Evaluate}},
	{"`", {Instruction::Undo}},
	{"1", {std::pair<int,int>{0,0}}},
	{"2", {std::pair<int,int>{0,1}}},
	{"3", {std::pair<int,int>{0,2}}},
	{"4", {std::pair<int,int>{0,3}}},
	{"q", {std::pair<int,int>{1,0}}},
	{"w", {std::pair<int,int>{1,1}}},
	{"e", {std::pair<int,int>{1,2}}},
	{"r", {std::pair<int,int>{1,3}}},
	{"a", {std::pair<int,int>{2,0}}},
	{"s", {std::pair<int,int>{2,1}}},
	{"d", {std::pair<int,int>{2,2}}},
	{"f", {std::pair<int,int>{2,3}}},
	{"z", {std::pair<int,int>{3,0}}},
	{"x", {std::pair<int,int>{3,1}}},
	{"c", {std::pair<int,int>{3,2}}},
	{"v", {std::pair<int,int>{3,3}}},
};

const Table<Variant_Type, int> default_tab_type_indexes{
	/* should instructions be on a tab? they are no longer elements
	{Instruction::Skip, 0},
	{Instruction::Undo, 0},
	{Instruction::Redo, 0},
	{Instruction::Cancel, 0},
	{Instruction::Termination, 0},
	*/

	// success feels super broad...
	{Variant_Type::Success, 0},

	// we are not allowing use of the Any type during gameplay command building
	// because it prohibits meaningful suggestions
	{Variant_Type::Any, -1},

	// used for conditionals, is this also used for filter functions?
	// could also filter by picking items from a list
	// or every filter could be its own element which by convention also takes a size
	{Variant_Type::Bool, 1},

	{Variant_Type::Number, 2},
	{Variant_Type::Digit, 2},

	{Variant_Type::ValueName, 3},
	{Variant_Type::Letter, 3}, // letters should probably not be hotkeyed? and just read literally

	{Variant_Type::Action_Type, 4}, // implicitly creates a Step of Type
	{Variant_Type::Action_Step, 4},

	{Variant_Type::Unit_Type, 5},
	{Variant_Type::Unit_Attribute, 5},

	{Variant_Type::UnitID, 6},
	{Variant_Type::Unit_Group, 6},

	// should Energy and Seconds be variant_types at all?
	{Variant_Type::Seconds, 7},
	{Variant_Type::Energy, 7},

	{Variant_Type::Point, 8},
	{Variant_Type::Direction, 8},
	{Variant_Type::Line, 8},
	{Variant_Type::Area, 8},
};

const std::string horizontal_line = "––––––––––––––––––––––––––––––––––––––––";

// @Feature different default_hotkeys for numbers of columns/rows

CardBuilder::CardBuilder(Context & context)
	: columns{default_columns}
	, rows{default_rows}
	, hotkeys{default_hotkeys}
	, context{context}
	, tabs{10}
	, tab_type_indexes{default_tab_type_indexes}
	, active_tab_index{0}
	, active_tab_has_allowed{true}
	, page_row_offset{0}
	, priority_next_count{0}
{
	SetupTabs(context.player->exposed_elements->GetAllTokens());
	PickTabBasedOnContextState();
}

void CardBuilder::SetupTabs(std::vector<ElementToken> tokens)
{
	// first tab is for punctuation, for now
	if (tabs.size() == 0)
	{
		tabs.emplace_back();
	}
	for(auto&& token : tokens)
	{
		if (tab_type_indexes.count(token.type) == 0)
		{
			tab_type_indexes[token.type] = tabs.size();
			tabs.emplace_back();
		}
		if (tab_type_indexes[token.type] == -1)
		{
			// this token is not of a public type, i.e. Command
			continue;
		}
		auto & tab = tabs[tab_type_indexes[token.type]];
		if (tab.tokens.size() == 0
			|| tab.tokens.back().size() == columns)
		{
			tab.tokens.emplace_back();
		}
		tab.tokens.back().emplace_back(token);
	}
}

ErrorOr<Success> CardBuilder::HandleInput(std::string input)
{
	if (hotkeys.count(input) == 0)
	{
		auto result = context.GetTokenForName({input});
		if (result.IsError())
		{
			return Error("Input not recognized as either hotkey or ElementName");
		}
		else
		{
			return HandleToken(result.GetValue());
		}
	}
	else
	{
		auto & key = hotkeys.at(input);
		if (std::holds_alternative<Instruction>(key))
		{
			return HandleInstruction(std::get<Instruction>(key));
		}
		if (std::holds_alternative<ElementToken>(key))
		{
			return HandleToken(std::get<ElementToken>(key));
		}
		else if (std::holds_alternative<TabNav>(key))
		{
			TabNav operation = std::get<TabNav>(key);
			switch(operation)
			{
				case TabNav::GoToType:
					return Error("GoToType is unimplemented");
				case TabNav::Left:
					SwitchToTab((active_tab_index + tabs.size() - 1) % tabs.size());
					return Success();
				case TabNav::Right:
					SwitchToTab((active_tab_index + 1) % tabs.size());
					return Success();
				case TabNav::NextHighestPriority:
				case TabNav::MoreOptionsForCurrentType:
				case TabNav::Back:
					RepeatTabOperationUntilContainsAllowed(operation);
					return Success();
				default:
					return Error("Invalid TabNav value");
			}
		}
		else // this is a tab dependent hotkey
		{
			auto index = std::get<std::pair<int, int> >(key.input);
			auto & tab = tabs[active_tab_index];
			if (tab.tokens.size() <= index.first + page_row_offset)
			{
				return Error("Hotkey is out of tab bounds, row");
			}
			else if (tab.tokens[index.first].size() <= index.second)
			{
				return Error("Hotkey is out of tab bounds, column");
			}
			return HandleToken(tab.tokens[index.first][index.second]);
		}
	}
}

ErrorOr<Success> CardBuilder::HandleToken(ElementToken token)
{
	if (!IsAllowed(token))
	{
		return Error("This token type is not allowed");
	}
	
	CHECK_RETURN(AppendElement(
		CHECK_RETURN(GetNewCommandElement(token.name))
	));
	BreakUndoChain(token);
	RefreshAllowedTypes();
	// feels a little weird to preempt adding 1 inside of Repeat...
	priority_next_count = -1;
	RepeatTabOperationUntilContainsAllowed(TabNav::NextHighestPriority);
	return Success();
}

ErrorOr<Success> CardBuilder::HandleInstruction(Instruction instruction)
{
	switch(instruction)
	{
		case Instruction::Skip:
			skip_count += 1;
			allowed.total_instruction[ET::Skip] -= 1;
			BreakUndoChain(token);
			return Success();
		case Instruction::Undo:
			return PerformUndo();
		case Instruction::Redo:
			return PerformRedo();
		case Instruction::Evaluate:
			// @Incomplete what should we do if we can't terminate?
			CHECK_RETURN(command->Evaluate(context));
			return InitNewCommand();
		case Instruction::Cancel:
			// command = CHECK_RETURN(GetNewCommandElement("Command"));
			return InitNewCommand();
	}
}


ErrorOr<Success> CardBuilder::InitNewCommand()
{
	command = CHECK_RETURN(GetNewCommandElement({"Command"}));
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


void Context::RefreshAllowedTypes()
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

ErrorOr<Success> Context::PerformUndo()
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

ErrorOr<Success> Context::PerformRedo()
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

void Context::BreakUndoChain(ElementToken token)
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

bool Context::IsAllowed(ElementToken token)
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

void Context::GetAllowedNextElements(Set<ElementName> & allowed)
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

ErrorOr<Success> Context::AppendElement(value_ptr<Element>&& next)
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


void CardBuilder::RepeatTabOperationUntilContainsAllowed(TabNav operation)
{
	// that priority_next_count goes up/down by more than one with each operation
	// feels pretty bad... but going over each possibility and only counting it if
	// there are allowed elements feels expensive.
	do
	{
		switch(operation)
		{
			case TabNav::Left:
				SwitchToTab((active_tab_index - 1) % tabs.size());
				break;
			case TabNav::Right:
				SwitchToTab((active_tab_index + 1) % tabs.size());
				break;
			case TabNav::NextHighestPriority:
				priority_next_count += 1;
				PickTabBasedOnContextState();
				break;
			case TabNav::MoreOptionsForCurrentType:
				SwitchToNextPageOnTab();
				break;
			case TabNav::Back:
				{
					// @Feature TabNavBack
					priority_next_count -= 1;
					bool hit_end = priority_next_count <= 0;
					if (hit_end)
					{
						priority_next_count = 0;
					}
					PickTabBasedOnContextState();
					if (hit_end)
					{
						return;
					}
				}
				break;
			default:
				break;
		}
	} while (active_tab_index != 0 && !active_tab_has_allowed);
}

void CardBuilder::PickTabBasedOnContextState()
{
	// @Implement PriorityAppend
	AllowedTypes remaining;
	if (context.skip_count == 0)
	{
		remaining = context.allowed;
	}
	else
	{
		AllowedTypes skipped;
		for (auto && allowed : context.allowed.priority)
		{
			int total_skipped = skipped.total_right[allowed.type]
				+ skipped.total_left[allowed.type];
			if (total_skipped < context.skip_count)
			{
				skipped.Append(allowed);
			}
			else
			{
				remaining.Append(allowed);
			}
		}
		remaining.total_instruction = context.allowed.total_instruction;
	}
	int priority_length = remaining.priority.size();
	
	if (priority_next_count >= priority_length)
	{
		// if we next over all options, all that's left is instructions
		// this covers a priority_length of 0
		SwitchToTab(0);
	}
	else
	{
		// next should change tab, so we keep track of the tab index rather than the type
		int current_tab_index = tab_type_indexes[remaining.priority[0].type];
		int priority_next = priority_next_count;
		for (auto & allowed : remaining.priority)
		{
			int tab_index = tab_type_indexes[allowed.type];
			if (tab_index < 0)
			{
				continue;
			}
			if (tab_index != current_tab_index)
			{
				priority_next -= 1;
				current_tab_index = tab_index;
			}
			if (priority_next == 0)
			{
				SwitchToTab(current_tab_index);
				return;
			}
		}

		// fallback because of the tracking of type changing above
		// we might have missed on the initial if condition
		SwitchToTab(0);
	}
}

void CardBuilder::SwitchToTab(int index)
{
	active_tab_index = index % tabs.size();
	auto & tab = tabs[active_tab_index];
	// should we always reset page row offset?
	// automatically scroll to first page_row_offset with a value?
	// could consider caching this on append element, or at least when calculated
	for (int offset = 0; offset < tab.tokens.size(); offset += rows)
	{
		page_row_offset = offset;
		int last_row = std::min(page_row_offset + rows,
			static_cast<int>(tab.tokens.size()));

		for (int r = page_row_offset; r < last_row; r++)
		{
			for(int c = 0; c < tab.tokens[r].size(); c++)
			{
				if(context.IsAllowed(tab.tokens[r][c]))
				{
					active_tab_has_allowed = true;
					return;
				}
			}
		}
	}
	
	// none of the offsets had an allowed value
	page_row_offset = 0;
	active_tab_has_allowed = false;
}

void CardBuilder::SwitchToNextPageOnTab()
{
	// should this skip to next allowed elements or not?
	page_row_offset += rows;

	if (page_row_offset >= tabs[active_tab_index].tokens.size())
	{
		page_row_offset = 0;
	}
}

std::string CardBuilder::MakeCurrentTabPrintString()
{
	std::string output;
	std::string line;
	for (int i = 0; i < tabs.size(); i++)
	{
		std::string tab_name = [&]() -> std::string {
			for (auto&& pair : tab_type_indexes)
			{
				if (pair.second != i)
				{
					continue;
				}
				if (Contains(Context::instruction_element_types, pair.first))
				{
					return "Instructions";
				}
				else
				{
					return ToString(pair.first);
				}
			}
			return "?";
		}();
		if (line.size() > 0)
		{
			line += "\t";
		}
		if (i == active_tab_index)
		{
			line += KCYN + tab_name + RST;
		}
		else
		{
			line += tab_name;
		}
		if (line.size() > 50)
		{
			output += line + "\n";
			line = "";
		}
	}
	if (!line.empty())
	{
		output += line + "\n";
		line = "";
	}
	output += horizontal_line + "\n";
	auto & tab = tabs[active_tab_index];
	int last_row = std::min(page_row_offset + rows, static_cast<int>(tab.tokens.size()));
	for (int r = page_row_offset; r < last_row; r++)
	{
		for(int c = 0; c < tab.tokens[r].size(); c++)
		{
			if (c > 0)
			{
				output += "\t";
			}
			if (!context.IsAllowed(tab.tokens[r][c]))
			{
				output += KRED + tab.tokens[r][c].name.value + RST;
			}
			else
			{
				output += tab.tokens[r][c].name.value;
			}
			
		}

		output += "\n";
	}
	return output;
}

std::vector<ElementToken> CardBuilder::MakeTokensFromDictionary(Dictionary dict)
{
	std::vector<ElementToken> tokens;
	tokens.reserve(dict.size());
	for (const auto & pair : dict)
	{
		tokens.push_back(ElementToken{pair.value.name, pair.value.type});
	}

}

} // namespace Command
