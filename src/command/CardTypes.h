#pragma once
#ifndef COMMAND_CARD_TYPES_H
#define COMMAND_CARD_TYPES_H

#include <queue>
#include <vector>
#include <utility>

#include "ElementName.h"
#include "Variant.h"

namespace Command
{

struct ElementToken
{
	ElementName name;
	Variant_Type type;
};

enum class Instruction
{
	Evaluate,
	Cancel,
	Skip,
	Undo,
	Redo,
};

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
	Back,
};

using HotKeyLoc = std::pair<int,int>;

// @Feature ordering/placing of tokens on tab
using TabTokenMapping = std::vector<std::vector<ElementToken>>;

using CardInput = std::variant<Instruction, TabNav, ElementToken, HotKeyLoc>;
}


#endif // COMMAND_CARD_TYPES_H
