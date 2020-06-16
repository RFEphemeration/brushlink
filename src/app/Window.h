#pragma once
#ifndef BRUSHLINK_WINDOW_H
#define BRUSHLINK_WINDOW_H

#include "tigr.h"

namespace Brushlink
{

struct Window_Settings
{
	int width{580};
	int height{320};
	std::string title{"BrushLink"};
	TPixel clear_color{0xa0, 0x90, 0x80, 0x255};
	Dimensions world_portion{260, 0, 320, 320};
};

struct Window
{
	Window_Settings settings;
	shared_ptr<Tigr> screen;

	Window(Window_Settings settings = Window_Settings{})
		: settings{settings}
		, screen{nullptr, TigrDeleter{}}
	{
		screen.reset(tigrWindow(
			settings.width,
			settings.height,
			settings.title.c_str(),
			0
		));
	}

	bool Closed()
	{
		return tigrClosed(screen.get());
	}

	void Clear()
	{
		tigrClear(screen.get(), settings.clear_color);
	}

	void Present()
	{
		tigrUpdate(screen.get());
	}
};

} // namespace Brushlink

#endif // BRUSHLINK_WINDOW_H
