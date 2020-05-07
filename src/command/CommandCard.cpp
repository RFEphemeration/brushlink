#include "CommandCard.h"

#include "TerminalExtensions.h"

namespace Command
{

const int default_columns = 4;
const int default_rows = 4;
const Table<std::string, CommandCard::CardInput> default_hotkeys{
	{"Tab", {TabNav::NextHighestPriority}},

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

// @Feature different default_hotkeys for numbers of columns/rows

CommandCard::CommandCard(CommandContext & context)
	: columns{default_columns}
	, rows{default_rows}
	, hotkeys{default_hotkeys}
	, context{context}
	, tabs{}
	, tab_type_indexes{}
	, active_tab_index{0}
	, page_row_offset{0}
	, priority_next_count{0}
{
	SetupTabs(context.GetAllTokens());
	PickTabBasedOnContextState();
}

void CommandCard::SetupTabs(std::vector<ElementToken> tokens)
{
	// first tab is for punctuation, for now
	tabs.emplace_back();
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
			CHECK_RETURN(context.HandleToken(result.GetValue()));
			priority_next_count = 0;
			PickTabBasedOnContextState();
			return Success();
		}
	}
	else
	{
		auto & key = hotkeys.at(input);
		if (std::holds_alternative<ElementToken>(key.input))
		{
			CHECK_RETURN(context.HandleToken(std::get<ElementToken>(key.input)));
			priority_next_count = 0;
			PickTabBasedOnContextState();
			return Success();
		}
		else if (std::holds_alternative<TabNav>(key.input))
		{
			switch(std::get<TabNav>(key.input))
			{
				case TabNav::GoToType:
					return Error("GoToType is unimplemented");
				case TabNav::Left:
					SwitchToTab((active_tab_index - 1) % tabs.size());
					return Success();
				case TabNav::Right:
					SwitchToTab((active_tab_index + 1) % tabs.size());
					return Success();
				case TabNav::NextHighestPriority:
					priority_next_count += 1;
					PickTabBasedOnContextState();
					return Success();
				case TabNav::MoreOptionsForCurrentType:
					SwitchToNextPageOnTab();
					return Success();
				case TabNav::Back:
					// @Feature TabNavBack
					return Error("TabNav Back is unimplemented");
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
			CHECK_RETURN(context.HandleToken(tab.tokens[index.first][index.second]));
			priority_next_count = 0;
			PickTabBasedOnContextState();
			return Success();
		}
	}
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
		ElementType::Enum current = remaining.priority[0].type;
		int priority_next = priority_next_count;
		for(int i = 0; i < priority_length; i++)
		{
			// is next by type, and skip for disambiguating between arguments of the same type?
			// or should you be able to use next instead of skip?
			// in which case we don't really need skip
			if (remaining.priority[i].type != current)
			{
				priority_next -= 1;
				current = remaining.priority[i].type;
			}
			if (priority_next == 0)
			{
				SwitchToTab(tab_type_indexes[current]);
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
	int old_active = active_tab_index;
	active_tab_index = index % tabs.size();
	if (old_active != active_tab_index)
	{
		// should we always reset page row offset?
		page_row_offset = 0;
	}
}

void CommandCard::SwitchToNextPageOnTab()
{
	page_row_offset += rows;

	if (page_row_offset >= tabs[active_tab_index].tokens.size())
	{
		page_row_offset = 0;
	}
}

std::string CommandCard::MakeCurrentTabPrintString()
{
	std::string output;
	output += "Tab " + std::to_string(active_tab_index) + "\n";
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
