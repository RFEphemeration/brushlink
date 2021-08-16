import os
import pyglet
from game import Match
from app.ui import *

pyglet.resource.path = [os.path.dirname(__file__) + '/resources/']
pyglet.resource.reindex()

class App():
	def __init__(self, initial):
		self.screens = {}
		self.elapsed_time = 0.0
		self.current_screen = initial

	def draw(self):
		self.screens[self.current_screen].draw()

	def change_state(self, target):
		self.current_screen = target
		self.screens[self.current_screen].on_enter();

	def show_popup(self, name):
		self.screens[self.current_screen].show_popup(name)

	def hide_popup(self, name):
		self.screens[self.current_screen].hide_popup(name)

	def update(self, dt):
		self.elapsed_time += dt
		self.screens[self.current_screen].update(dt)

	def on_mouse_press(self, x, y, buttons, modifiers):
		self.screens[self.current_screen].on_mouse_press(x, y, buttons, modifiers)

	def exit(self):
		pyglet.app.exit()

	def delete_screens(self):
		self.screens = {}

	def add_screen(self, screen):
		self.screens[screen.name] = screen
		screen.set_app(self)
		if screen.name == self.current_screen:
			screen.on_enter()

	def update_window_size(self, size):
		window_calc = ComputedSize(0, 0, size[0], size[1])
		for screen in self.screens:
			self.screens[screen].update_window_size(window_calc)


class Screen():
	def __init__(self, name, wigits, exit_time = None, on_exit = None):
		self.name = name
		self.wigits = wigits
		self.exit_time = exit_time
		self.on_exit = on_exit
		self.elapsed_time = 0.0
		self.app = None

	def set_app(self, app):
		self.app = app
		for wigit in self.wigits:
			if getattr(wigit, 'set_app', None):
				wigit.set_app(app)

	def on_enter(self):
		self.elapsed_time = 0.0

	def update(self, dt):
		self.elapsed_time += dt
		if self.exit_time is not None and self.elapsed_time >= self.exit_time:
			self.on_exit(self)

	def draw(self):
		for wigit in self.wigits:
			wigit.draw()

	def on_mouse_press(self, x, y, buttons, modifiers):
		for wigit in self.wigits:
			if getattr(wigit, 'on_mouse_press', None):
				wigit.on_mouse_press(x, y, buttons, modifiers)

	def update_window_size(self, window_calc):
		for wigit in self.wigits:
			if getattr(wigit, 'update_size', None):
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
		self.screens[self.current_screen].hide_popup(name)


class MatchScreen():
	def __init__(self, match, player_id):
		self.match = match
		self.player_id = player_id

	def draw(self):
		glPushMatrix()
		camera_center = self.match.players[self.player_id].camera_center
		glTranslatef(camera_center[0], camera_center[1])

		self.match.draw(self.player_id)

		glPopMatrix()


window = pyglet.window.Window(fullscreen=True)

app = App(initial='title')

def init_screens(app):
	screens = {}
	app.add_screen(Screen('layout_test', [
		Panel(
			box=Box(top=(10,"px"),left=(10,"px"),bottom=(10,"px"), right=(10,"px")),
			wigits = [
				Panel([], color=(100,50,50),
					box=Box(hor=(0,"px"),vert=(0,"px"),height=(200,"px"), width=(200,"px")))
			])
		]))
	app.add_screen(Screen('title', [
		pyglet.text.Label(
			'BrushLink',
			font_size=36,
			x=window.width // 2,
			y=window.height // 2 + 40,
			anchor_x='center',
			anchor_y='center'),
		pyglet.text.Label(
			'by ephemeration games',
			font_size=24,
			x=window.width // 2,
			y=window.height // 2 - 20,
			anchor_x='center',
			anchor_y='center'),
		],
		exit_time=0.5,
		on_exit=lambda s: s.app.change_state('main')))
	app.add_screen(Screen('main', [
		pyglet.text.Label(
			'BrushLink',
			font_size=36,
			x=window.width // 2,
			y=window.height - 40,
			anchor_x='center',
			anchor_y='center'),
		Button(
			"settings",
			box=Box(hor=(0,"px"),vert=(100,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: b.app.change_state('settings')),
		Button(
			"play",
			box=Box(hor=(0,"px"),vert=(50,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: b.app.change_state('game')),
		Button(
			"exit",
			box=Box(hor=(0,"px"),vert=(0,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: b.app.change_state('exit')),
		]))
	app.add_screen(Screen('settings', [
		pyglet.text.Label(
			"Settings",
			font_size=36,
			x=window.width // 2,
			y=window.height - 40,
			anchor_x='center',
			anchor_y='center'),
		Button(
			'back',
			box=Box(hor=(0,"px"),vert=(100,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: b.app.change_state('main')),
		]))
	game_menu = Panel(name='menu', enabled=False, wigits=[
		Button(
			"resume",
			box=Box(bottom=(20,"px"),right=(20,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: game_menu.hide()),
		],
		box=Box(hor=(0,"px"),vert=(0,"px"),width=(400,"px"),height=(300,"px")))
	app.add_screen(Screen('game', [
		game_menu,
		Button(
			"menu",
			box=Box(bottom=(20,"px"),right=(20,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: game_menu.show()),
		]))
	app.add_screen(Screen('exit', [],
		exit_time=0.0,
		on_exit=lambda s: s.app.exit()
	))

init_screens(app)

app.update_window_size(window.get_size())

@window.event
def on_draw():
	window.clear()
	app.draw()

@window.event
def on_mouse_press(x, y, buttons, modifiers):
	app.on_mouse_press(x, y, buttons, modifiers)

pyglet.clock.schedule_interval(app.update, 1.0/12.0)
pyglet.app.run()
