#ifndef BRUSHLINK_COMMAND_CARD_H
#define BRUSHLINK_COMMAND_CARD_H

#include "ElementType.h"
#include "CommandContext.h"

namespace Command
{

enum class TabNav
{
	// immediately navigate to Element_Type tab, then input there consumed by Card
	 // how to implement that tab?
	GoToType, 
	Left,
	Right,
	// could this implicitly append skips? if you've already passed one of the same type
	NextHighestPriority,
	MoreOptionsForCurrentType,
	Back
};

struct CommandCard
{
	struct Tab
	{
		// used for constructing buildings or units of particular types
		// maybe these shouldn't collapse because then we couldn't have two different types of buildings selected
		// bool collapse_allowed;
		// @Feature ordering/placing of tokens on tab
		std::vector<std::vector<ElementToken>> tokens;
	};

	struct CardInput
	{
		// direct token, card navigation, or tab relative hotkey
		// tab relative hotkey is row, column
		std::variant<ElementToken, TabNav, std::pair<int,int>> input;

		CardInput(std::variant<ElementToken, TabNav, std::pair<int,int>> input)
			: input(input)
		{ }
	};

	// these should be user setting controlled

	int columns;
	int rows;
	Table<std::string, CardInput> hotkeys;

	// these are fixed around for the duration of the runtime
	// could consider making them const

	CommandContext & context;
	std::vector<Tab> tabs;
	// are punctuation elements always available?
	// should the tabs automatically be split by type?
	// or could some be combined?
	Table<ElementType::Enum, int> tab_type_indexes;

	// these change as you input

	int active_tab_index;
	// reset when you switch tabs
	int page_row_offset;
	// reset when you append an element
	int priority_next_count;

	CommandCard(CommandContext & context);

	void SetupTabs(std::vector<ElementToken> tokens);

	ErrorOr<Success> HandleInput(std::string input);
	void PickTabBasedOnContextState();
	void SwitchToTab(int index);
	void SwitchToNextPageOnTab();

	std::string MakeCurrentTabPrintString();
};

} // namespace Command

#endif // BRUSHLINK_COMMAND_CARD_H
