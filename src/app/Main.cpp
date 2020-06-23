
#include <iostream> 
#include <chrono>
#include <thread>

#include "Window.h"
#include "Game.h"
#include "Input.h"

using namespace Brushlink;

int main(int argc, char *argv[])
{
	std::cout << "startup" << std:: endl;
	Window window;
	// Input input;
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
		const auto game_start = std::chrono::steady_clock::now();
		auto game_current = game_start;
		auto tick_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::duration<double> {
				1.0  / static_cast<double>(game.settings.speed.value)
			}
		);
		auto next_tick = game_start;
		while(!window.Closed()
			&& !game.IsOver())
		{
			game_current = std::chrono::steady_clock::now();
			if (game_current > next_tick)
			{
				game.Tick();
				// will this cause drift? is that a problem?
				next_tick = next_tick + tick_duration;
			}
			window.Clear();
			game.Render(window.screen.get(), window.settings.world_portion);
			window.PresentAndUpdate();
			//input.Process(window.screen, window.GetKeyChanges());
			// where should sleep go wrt to event processing and rendering?
			// present and update pumps the event queue...
			std::this_thread::sleep_until(next_tick);
			
		}
		std::cout << "game over" << std::endl;
	
	}

	return 0;
}
