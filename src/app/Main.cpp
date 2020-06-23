
#include <iostream> 

#include "Window.h"
#include "Game.h"

using namespace Brushlink;

int main(int argc, char *argv[])
{
	std::cout << "startup" << std:: endl;
	Window window;
	while (!window.Closed())
	{
		/* todo: input processing,
			starting new game,
			menus/navigation,
			changing settings, // this means globals or ui state machine handoffs
			code editor
		*/
		std::cout << "new game" << std::endl;
		Game game;
		game.Initialize();
        int i = 0;
		while(!window.Closed())
		{
			window.Clear();
            if (i++ > 20)
            {
                game.Tick();
                i -= 20;
            }
			game.Render(window.screen.get(), window.settings.world_portion);
			window.PresentAndUpdate();

			if (game.IsOver())
			{
				std::cout << "game over" << std::endl;
				break;
			}
			// sleep, where should this go?
		}
	
	}

	return 0;
}
