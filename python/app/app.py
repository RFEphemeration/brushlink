import os
import pyglet
from game import Match
from app.ui import *
from app.player_prefs import *

pyglet.resource.path = [os.path.dirname(__file__) + '/resources/']
pyglet.resource.reindex()

class App():
	def __init__(self):
		self.screens = {}
		self.elapsed_time = 0.0
		self.current_screen = None
		self.window_size = (0,0,0,0)
		self.player_prefs = PlayerPrefs()

	def draw(self):
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

	def on_mouse_press(self, x, y, buttons, modifiers):
		self.current_screen.on_mouse_press(x, y, buttons, modifiers)

	def exit(self):
		pyglet.app.exit()

	def delete_screens(self):
		self.screens = {}

	def add_screen(self, name, constructor):
		self.screens[name] = constructor

	def update_window_size(self, size):
		self.window_size = ComputedSize(0, 0, size[0], size[1])
		if self.current_screen is not None:
			self.current_screen.update_window_size(self.window_size)

	def get_player_prefs(self):
		return self.player_prefs


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
	def __init__(self, name, wigits, command_card, command_tab_indexes):
		super().__init__(name, wigits)
		#self.match = match
		#self.player_id = player_id
		self.command_card = command_card
		self.command_tab_indexes = command_tab_indexes

	def command_change_tab(self, element_type):
		self.command_card.set_active_tab(self.command_tab_indexes[element_type])

	def draw(self):
		super().draw()
		"""
		glPushMatrix()
		camera_center = self.match.players[self.player_id].camera_center
		glTranslatef(camera_center[0], camera_center[1])

		self.match.draw(self.player_id)

		glPopMatrix()
		"""

