#include "Input.h"

namespace Brushlink
{

Input_Result Input::ProcessInput(
	const Key_Changes & keys,
	const Modifiers_State & modifiers,
	const Mouse_State & mouse)
{
	bool mouse_down = IsDown(mouse.left) || IsDown(mouse.right);
	if (mouse_down)
	{
		if (mouse_line.empty()
			|| mouse.window_position != mouse_line.back())
		{
			mouse_line.push_back(mouse.window_position);
		}
	}
	Input_Result result { Input_Result::NoUpdateNeeded };
	for (auto & listener : listeners)
	{
		auto listener_result = listener->operator()(
			keys,
			modifiers,
			mouse,
			mouse_line);
		if (listener_result == Input_Result::UpdateRequested)
		{
			result = listener_result;
		}
	}
	if (!mouse_down)
	{
		mouse_line.clear();
	}
	return result;
}

} // namespace Brushlink
