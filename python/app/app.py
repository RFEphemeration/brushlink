import os
import pyglet
from game import Match
from app.player_prefs import *
from app.command_card import *

pyglet.resource.path = [os.path.dirname(__file__) + '/resources/']
pyglet.resource.reindex()

class App():

	instance = None

	def __init__(self, window):
		self.screens = {}
		self.elapsed_time = 0.0
		self.window = window
		self.current_screen = None
		self.player_prefs = PlayerPrefs()
		self.window_size = (0,0,0,0)
		self.update_window_size(*self.window.get_size())
		self.coroutines = []
		self.window.push_handlers(
			on_resize=self.update_window_size,
			on_mouse_press=self.on_mouse_press,
			on_draw=self.draw,
			on_key_press=self.on_key_press)

	def __del__(self):
		self.window.pop_handlers()

	def draw(self):
		self.window.clear()
		self.current_screen.draw()

	def change_state(self, target):
		self.current_screen = self.screens[target](self)
		self.current_screen.update_window_size(self.window_size)
		self.current_screen.enter()

	def show_popup(self, name):
		self.current_screen.show_popup(name)

	def hide_popup(self, name):
		self.current_screen.hide_popup(name)

	def update(self, dt):
		self.elapsed_time += dt
		self.current_screen.update(dt)
		remaining_coroutines = []
		for coroutine in self.coroutines:
			try:
				coroutine.send(dt)
				remaining_coroutines.append(coroutine)
			except StopIteration:
				pass

		self.coroutines = remaining_coroutines

	def on_mouse_press(self, x, y, buttons, modifiers):
		self.current_screen.on_mouse_press(x, y, buttons, modifiers)

	def on_key_press(self, symbol, modifiers):
		pass

	def exit(self):
		pyglet.app.exit()

	def delete_screens(self):
		self.screens = {}

	def add_screen(self, name, constructor):
		self.screens[name] = constructor

	def update_window_size(self, width, height):
		self.window_size = ComputedSize(0, 0, width, height)
		if self.current_screen is not None:
			self.current_screen.update_window_size(self.window_size)

	def get_player_prefs(self):
		return self.player_prefs

	def start_coroutine(self, coroutine):
		# calling next immediately because the default behavior is to send dt in update
		next(coroutine, None)
		self.coroutines.append(coroutine);

	def stop_coroutine(self, coroutine):
		self.coroutine.remove(coroutine)


class Screen():
	def __init__(self, name, wigits, exit_time=None, on_exit=None, on_enter=None):
		self.name = name
		self.wigits = wigits
		self.exit_time = exit_time
		self.on_exit = on_exit
		self.on_enter = on_enter
		self.elapsed_time = 0.0

	def enter(self):
		self.elapsed_time = 0.0
		if self.on_enter is not None:
			self.on_enter(self)

	def update(self, dt):
		self.elapsed_time += dt
		if self.on_exit is not None and self.exit_time is not None and self.elapsed_time >= self.exit_time:
			self.on_exit(self)

	def draw(self):
		for wigit in self.wigits:
			wigit.draw()

	def on_mouse_press(self, x, y, buttons, modifiers):
		for wigit in reversed(self.wigits):
			if hasattr(wigit, 'on_mouse_press'):
				consumed = wigit.on_mouse_press(x, y, buttons, modifiers)
				if consumed:
					return True
		return False

	def update_window_size(self, window_calc):
		for wigit in self.wigits:
			if hasattr(wigit, 'update_size'):
				wigit.update_size(window_calc, window_calc)

	def show_popup(self, name):
		for wigit in self.wigits:
			if getattr(wigit, 'name', None) == name:
				wigit.enabled = True
				break

	def hide_popup(self, name):
		for wigit in self.wigits:
			if getattr(wigit, 'name', None) == name:
				wigit.enabled = False
				break

	def hide_popup(self, name):
		self.current_screen.hide_popup(name)


class MatchScreen(Screen):
	def __init__(self, name, wigits, command_card, window):
		super().__init__(name, wigits)
		#self.match = match
		#self.player_id = player_id
		self.command_card = command_card
		self.window = window
		self.window.push_handlers(on_key_press=self.command_card.on_key_press)

	def draw(self):
		super().draw()
		"""
		glPushMatrix()
		camera_center = self.match.players[self.player_id].camera_center
		glTranslatef(camera_center[0], camera_center[1])

		self.match.draw(self.player_id)

		glPopMatrix()
		"""

	def __del__(self):
		self.window.pop_handlers()


# this is moved to the end to avoid circular imports, which feels bad
from app.ui import *
