#pragma once
#ifndef BRUSHLINK_WINDOW_H
#define BRUSHLINK_WINDOW_H

#include <memory>
#include <string>
#include <string_view>

#include "TigrExtensions.h"

namespace Brushlink
{

using Farb::UI::Dimensions;
using Farb::UI::TigrDeleter;

struct Key_Changes
{
	std::string_view up_values;
	std::string_view down_values;
};

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
	char key_down_buffer[256];
	char key_up_buffer[256];

	Window(Window_Settings settings = Window_Settings{})
		: settings{settings}
		, screen{tigrWindow(
			settings.width,
			settings.height,
			settings.title.c_str(),
			0)
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

	inline void PresentAndUpdate()
	{
		tigrUpdate(screen.get());
	}

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
};

} // namespace Brushlink

#endif // BRUSHLINK_WINDOW_H
