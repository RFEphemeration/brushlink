
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
	Input input;
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
		input.listeners["game"].reset(MakeCurriedMember(&Game::ReceiveInput, game));
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
			// could make this a while loop in order to jump multiple frames
			// but as is this will just render them all as quickly as possible
			// however we could end up rendering slower than the target framerate
			if (game_current > next_tick)
			{
				game.Tick();
				// will this cause drift? is that a problem?
				next_tick = next_tick + tick_duration;
			}
			// @Feature RenderRate faster than tick rate
			// especially for things like mouse input
			// which we could render on top of the world
			window.Clear();
			game.Render(window.screen.get(), window.GetWorldPortion());
			window.PresentAndUpdate();

			auto input_result = input.ProcessInput(
				window.GetKeyChanges(),
				window.GetModifiers(),
				window.GetMouseState());
			if (input_result == Input_Result::NoUpdateNeeded)
			{
				// where should sleep go wrt to event processing and rendering?
				// present and update pumps the event queue...
				// could also consider undershooting by a bit and spin locking
				// in order to avoid missing frames
				std::this_thread::sleep_until(next_tick);
			}
		}
		input.listeners.erase("game");
		std::cout << "game over" << std::endl;
	
	}

	return 0;
}
