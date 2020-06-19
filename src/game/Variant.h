#pragma once
#ifndef BRUSHLINK_VARIANT_H
#define BRUSHLINK_VARIANT_H

#include "Game_Time.h"
#include "Game_Basic_Types.h"
#include "Location.h"
#include "Action.h"
#include "Resources.h"

namespace Brushlink
{

// todo: custom struct/record, sum, and tuple types
// should vector and optional be included in variant?

using Variant = std::variant<
	Number,
	Digit,
	ValueName,
	Letter,
	Seconds,
	Action_Type,
	Unit_Type,
	Unit_Attribute,
	Unit_Group,
	Energy,
	Point,
	Direction,
	Line,
	Area>;

} // namespace Brushlink

#endif // BRUSHLINK_VARIANT_H


