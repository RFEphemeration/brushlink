#pragma once
#ifndef BRUSHLINK_WINDOW_H
#define BRUSHLINK_WINDOW_H

#include <memory>
#include <string>
#include <string_view>

#include "TigrExtensions.h"

#include "Input.h"

namespace Brushlink
{

using Farb::UI::Dimensions;
using Farb::UI::TigrDeleter;


struct Window_Settings
{
	int width{520};
	int height{320};
	std::string title{"BrushLink"};
	TPixel clear_color{0xa0, 0x90, 0x80, 0xFF};
	Dimensions world_portion{200, 0, 320, 320};
};

struct Window
{
	Window_Settings settings;
	std::shared_ptr<Tigr> screen;
	std::shared_ptr<Tigr> screen_buffer;
	char key_down_buffer[256]{0};
	char key_up_buffer[256]{0};
	int previous_mouse_buttons{0};

	Window(Window_Settings settings = Window_Settings{})
		: settings{settings}
		, screen{tigrWindow(
			settings.width,
			settings.height,
			settings.title.c_str(),
			TIGR_FARB_OVERSCALE_DOWNSIZE)
			, TigrDeleter{}
		}
		, screen_buffer{tigrBitmap(
			settings.width,
			settings.height)
			, TigrDeleter{}
		}
	{ }

	inline bool Closed()
	{
		return tigrClosed(screen.get());
	}

	inline void Clear()
	{
		tigrClear(screen.get(), settings.clear_color);
	}

	void PresentAndUpdate();

	Dimensions GetWorldPortion();

	inline Key_Changes GetKeyChanges()
	{
		unsigned int down_count = 0;
		unsigned int up_count = 0;
		tigrGetKeyChanges(screen.get(),
			&key_down_buffer, &down_count,
			&key_up_buffer, &up_count);
		return Key_Changes {
			{ key_down_buffer, down_count },
			{ key_up_buffer, up_count }
		};
	}

	inline Modifiers_State GetModifiers()
	{
		Tigr * bmp = screen.get();
		return Modifiers_State{
			tigrKeyHeld(bmp, TK_LSHIFT)   || tigrKeyHeld(bmp, TK_RSHIFT),
			tigrKeyHeld(bmp, TK_LCONTROL) || tigrKeyHeld(bmp, TK_RCONTROL),
			tigrKeyHeld(bmp, TK_LWIN)     || tigrKeyHeld(bmp, TK_RWIN),
			tigrKeyHeld(bmp, TK_LALT)     || tigrKeyHeld(bmp, TK_RALT),
		};
	}

	inline Mouse_State GetMouseState()
	{
		int mouse_buttons;
		Point mouse_location;
		tigrMouse(
			screen.get(),
			&mouse_location.x,
			&mouse_location.y,
			&mouse_buttons
		);
		Mouse_State state{
			FromDownHistory(previous_mouse_buttons & 1, mouse_buttons & 1),
			FromDownHistory(previous_mouse_buttons & 2, mouse_buttons & 2),
			mouse_location
		};
		previous_mouse_buttons = mouse_buttons;
		return state;
	}
};

} // namespace Brushlink

#endif // BRUSHLINK_WINDOW_H
