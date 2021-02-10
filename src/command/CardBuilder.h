#ifndef BRUSHLINK_CARD_BUILDER_H
#define BRUSHLINK_CARD_BUILDER_H

#include "Variant.h"

#include "CardTypes.h"
#include "Context.h"
#include "Element.hpp"

namespace Command
{

struct CardBuilder
{
	// #### these should be user setting controlled
	int columns;
	int rows;
	Table<std::string, CardInput> hotkeys;

	// #### these are fixed around for the duration of the runtime
	// could consider making them const
	Context & context;
	std::vector<Tab> tabs;
	// are instruction elements always available?
	// should the tabs automatically be split by type?
	// or could some be combined?
	Table<Variant_Type, int> tab_type_indexes;

	// #### these change as you input and represent the command card state
	int active_tab_index;
	bool active_tab_has_allowed;
	// reset when you switch tabs
	int page_row_offset;
	// reset when you append an element
	int priority_next_count;

	// #### these change as you input and represent the command you're building
	value_ptr<Element> command;

	AllowedTypes allowed;
	int skip_count;
	std::vector<ElementToken> undo_stack;
	int undo_count;
	std::vector<std::string> action_log;

	CardBuilder(Context & context);

	void SetupTabs(std::vector<ElementToken> tokens);

	ErrorOr<Success> HandleInput(std::string input);
	ErrorOr<Success> HandleToken(ElementToken token);
	ErrorOr<Success> HandleInstruction(Instruction instruction);
	void RepeatTabOperationUntilContainsAllowed(TabNav operation);
	void PickTabBasedOnContextState();
	void SwitchToTab(int index);
	void SwitchToNextPageOnTab();

	std::string MakeCurrentTabPrintString();

	ErrorOr<Success> InitNewCommand();
	void GetAllowedNextElements(Set<ElementName> & allowed);
	bool IsAllowed(ElementToken token);

	void RefreshAllowedTypes();
	ErrorOr<Success> PerformUndo();
	ErrorOr<Success> PerformRedo();
	void BreakUndoChain(ElementToken token);
	ErrorOr<Success> AppendElement(value_ptr<Element>&& next);
};

} // namespace Command

#endif // BRUSHLINK_CARD_BUILDER_H
