
#include "Window.h"
#include "Game.h"

int main(int argc, char *argv[])
{
	Window window;
	while (!window.Closed())
	{
		/* todo: input processing,
			starting new game,
			menus/navigation,
			changing settings, // this means globals or ui state machine handoffs
			code editor
		*/
		Game game;
		while(!window.Closed
			&& !game.IsOver())
		{
			window.Clear();
			game.Tick();
			game.Render(window.screen.get(), window.settings.world_portion);
		}
		// sleep, where should this go?
	}

	return 0;
}