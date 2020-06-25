#pragma once
#ifndef BRUSHLINK_INPUT_H
#define BRUSHLINK_INPUT_H

#include "MapReduce.hpp"

#include "Location.h"

namespace Brushlink
{

using namespace Farb;

struct Key_Changes
{
	std::string_view up_values;
	std::string_view down_values;
};

enum class Button_State
{
	Rest,
	Down,
	Held,
	Up,
};

inline Button_State FromDownHistory(bool was_down, bool is_down)
{
	if(was_down)
	{
		if(is_down)
		{
			return Button_State::Held;
		}
		return Button_State::Up;
	}
	if(is_down)
	{
		return Button_State::Down;
	}
	return Button_State::Rest;
};

inline bool IsDown(const Button_State & state)
{
	return state == Button_State::Down || state == Button_State::Held;
};

// up and down events handled through key_changes
struct Modifiers_State
{
	// shift, ctrl, cmd, alt
	bool shift;
	bool ctrl;
	bool cmd;
	bool alt;
};

struct Mouse_State
{
	Button_State left;
	Button_State right;
	Point window_position;
};


enum class Input_State
{
	CommandCard,
	Mouse,
	NameLiteral,
};

enum class Mouse_Behavior
{
	Unit,
	Point,
	DragLine,
	DragBox,
};

enum class Input_Result
{
	UpdateRequested,
	NoUpdateNeeded
};

using Input_Listener = Functor<
	Input_Result,
	const Key_Changes &,
	const Modifiers_State &,
	const Mouse_State &,
	const std::vector<Point> &>;

struct Input
{
	// modifiers combined with mouse movement for different locations
	// if modifiers also affect the command card, how to reconcile?
	// shift for direction? click is relative to selection, drag is absolute
	// click no modifiers for point (on unit for unit)
	// click drag no modifiers for box area
	// alt/option drag for line - if line is closed turn into perimeter area

	std::vector<value_ptr<Input_Listener>> listeners;

	// are these window positions? what about scrolling while drawing?
	std::vector<Point> mouse_line;

	Input_Result ProcessInput(
		const Key_Changes & keys,
		const Modifiers_State & modifiers,
		const Mouse_State & mouse);
};

} // namespace Brushlink

#endif // BRUSHLINK_INPUT_H
