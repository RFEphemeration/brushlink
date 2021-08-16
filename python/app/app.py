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


class Popup(pyglet.gui.WidgetBase):
	def __init__(self, name, wigits,
			box=Box(vert=(0,"px"),hor=(0,"px"),width=(20,"vw"), height=(40,"vh")),
			color=(100,100,100),
			enabled=False):
		super().__init__(0,0,0,0)
		self.enabled = enabled

		self.box = box
		self.background = pyglet.shapes.Rectangle(x=0, y=0, width=0, height=0, color=color)
		self.name = name
		self.wigits = wigits

	def draw(self):
		if not self.active:
			return
		self.background.draw()
		for wigit in self.wigits:
			wigit.draw()

	def on_mouse_press(self, x, y, buttons, modifiers):
		if not self.active:
			return
		for wigit in self.wigits:
			if getattr(wigit, 'on_mouse_press', None):
				wigit.on_mouse_press(x, y, buttons, modifiers)

	def show(self):
		self.active = True

	def hide(self):
		self.active = False

	def update_size(self, window_calc, parent_calc):
		old_size = self.box.calculated
		size = self.box.calculate(window_calc, parent_calc)
		if size == old_size:
			return
		size.update_widget(self)
		size.update_other(self.background)
		for wigit in self.wigits:
			if getattr(wigit, 'update_size', None):
				wigit.update_size(window_calc, size)


class Button(pyglet.gui.WidgetBase):
	def __init__(self, label, box, on_press, font_size = 18, color=(155,155,255)):
		self.box = box
		super().__init__(0,0,0,0);
		self.background = pyglet.shapes.Rectangle(x=0, y=0, width=0, height=0, color=color)
		self.label = pyglet.text.Label(label, font_size, x=0, y=0, anchor_x='center', anchor_y='center');
		self.on_press = on_press
		self.app = None

	def on_mouse_press(self, x, y, buttons, modifiers):
		if self._check_hit(x, y):
			self.on_press(self)

	def set_app(self, app):
		self.app = app

	def draw(self):
		self.background.draw()
		self.label.draw()

	def update_size(self, window_calc, parent_calc):
		old_size = self.box.calculated
		size = self.box.calculate(window_calc, parent_calc)
		if size == old_size:
			return

		size.update_widget(self)
		size.update_other(self.label)
		size.update_other(self.background)
		self.background.anchor_x = self.background.width // 2;
		self.background.anchor_y = self.background.height // 2;


window = pyglet.window.Window(fullscreen=True)

app = App(initial='title')

def init_screens(app):
	screens = {}
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
	game_menu = Popup('menu', [
		Button(
			"resume",
			box=Box(bottom=(20,"px"),right=(20,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: b.screen.hide_popup('menu')),
		],
		box=Box(hor=(0,"px"),vert=(0,"px"),width=(400,"px"),height=(300,"px")))
	app.add_screen(Screen('game', [
		Button(
			"menu",
			box=Box(bottom=(20,"px"),right=(20,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: b.screen.show_popup('menu')),
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
