#include "CommandCard.h"

#include "ContainerExtensions.hpp"
#include "TerminalExtensions.h"

namespace Command
{

const int default_columns = 4;
const int default_rows = 4;
const Table<std::string, CommandCard::CardInput> default_hotkeys{
	{"Tab", {TabNav::NextHighestPriority}},
	{"Back", {TabNav::Back}},
	{"Left", {TabNav::Left}},
	{"Right", {TabNav::Right}},

	// @Feature difference between hold shift and tap shift
	// @Bug modifier key input detection
	// could consider reserving shift for punctuation / tab nav input
	// or perhaps one of the other modifiers, maybe CTRL for commands, ALT for nav?
	{"Shift", {TabNav::MoreOptionsForCurrentType}},
	{"Escape", {ElementToken{ElementType::Cancel, "Cancel"}}},
	{"Space", {ElementToken{ElementType::Termination, "Termination"}}},
	{"`", {ElementToken{ElementType::Undo, "Undo"}}},
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

const Table<ElementType::Enum, int> default_tab_type_indexes{
	{ElementType::Skip, 0},
	{ElementType::Undo, 0},
	{ElementType::Redo, 0},
	{ElementType::Cancel, 0},
	{ElementType::Termination, 0},

	{ElementType::Command, -1},

	{ElementType::Action, 1},

	{ElementType::Condition, 2},

	{ElementType::Selector, 3}, // for Union and such
	{ElementType::Set, 3}, // could this be shared with selector?
	{ElementType::Filter, 3},
	{ElementType::Group_Size, 3},
	{ElementType::Superlative, 3},

	{ElementType::Location, 4},
	{ElementType::Point, 4},
	{ElementType::Line, 4},
	{ElementType::Area, 4},
	{ElementType::Direction, 4},

	{ElementType::Number, 5},
	{ElementType::Digit, 5},

	//Name
	//Letter

	{ElementType::Unit_Type, 6},
	{ElementType::Attribute_Type, 7},
	{ElementType::Ability_Type, 8},
	{ElementType::Resource_Type, 9},
};

const std::string horizontal_line = "––––––––––––––––––––––––––––––––––––––––";

// @Feature different default_hotkeys for numbers of columns/rows

CommandCard::CommandCard(CommandContext & context)
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
	SetupTabs(context.GetAllTokens());
	PickTabBasedOnContextState();
}

void CommandCard::SetupTabs(std::vector<ElementToken> tokens)
{
	// first tab is for punctuation, for now
	if (tabs.size() == 0)
	{
		tabs.emplace_back();
	}
	tab_type_indexes[ElementType::Skip] = 0;
	tab_type_indexes[ElementType::Undo] = 0;
	tab_type_indexes[ElementType::Redo] = 0;
	tab_type_indexes[ElementType::Cancel] = 0;
	tab_type_indexes[ElementType::Termination] = 0;
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

ErrorOr<Success> CommandCard::HandleInput(std::string input)
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
		if (std::holds_alternative<ElementToken>(key.input))
		{
			return HandleToken(std::get<ElementToken>(key.input));
		}
		else if (std::holds_alternative<TabNav>(key.input))
		{
			TabNav operation = std::get<TabNav>(key.input);
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

ErrorOr<Success> CommandCard::HandleToken(ElementToken token)
{
	CHECK_RETURN(context.HandleToken(token));
	// feels a little weird to preempt adding 1 inside of Repeat...
	priority_next_count = -1;
	RepeatTabOperationUntilContainsAllowed(TabNav::NextHighestPriority);
	return Success();
}

void CommandCard::RepeatTabOperationUntilContainsAllowed(TabNav operation)
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

void CommandCard::PickTabBasedOnContextState()
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

void CommandCard::SwitchToTab(int index)
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

void CommandCard::SwitchToNextPageOnTab()
{
	// should this skip to next allowed elements or not?
	page_row_offset += rows;

	if (page_row_offset >= tabs[active_tab_index].tokens.size())
	{
		page_row_offset = 0;
	}
}

std::string CommandCard::MakeCurrentTabPrintString()
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
				if (Contains(CommandContext::instruction_element_types, pair.first))
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

} // namespace Command
