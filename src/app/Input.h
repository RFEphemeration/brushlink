#pragma once
#ifndef BRUSHLINK_INPUT_H
#define BRUSHLINK_INPUT_H

namespace Brushlink
{

struct Input
{
	// modifiers combined with mouse movement for different locations
	// if modifiers also affect the command card, how to reconcile?
	// shift for direction? click is relative to selection, drag is absolute
	// click no modifiers for point (on unit for unit)
	// click drag no modifiers for box area
	// alt/option drag for line - if line is closed turn into perimeter area
};

} // namespace Brushlink

#endif // BRUSHLINK_INPUT_H
