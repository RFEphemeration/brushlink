import os
import pyglet
from game import Match

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
	def __init__(self, name, wigits, x, y, color=(100,100,100), width=400, height=600, active=False):
		self.background = pyglet.shapes.Rectangle(x=x, y=y, width=width, height=height, color=color)
		self.background.anchor_x = width // 2;
		self.background.anchor_y = height // 2;
		self.name = name
		self.wigits = wigits
		self.active = active

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


class Button(pyglet.gui.WidgetBase):
	def __init__(self, label, x, y, on_press,font_size = 18, color=(155,155,255), width=200, height=40):
		super().__init__(x - width // 2, y - height // 2, width, height);
		self.background = pyglet.shapes.Rectangle(x=x, y=y, width=width, height=height, color=color)
		self.background.anchor_x = width // 2;
		self.background.anchor_y = height // 2;
		self.label = pyglet.text.Label(label, font_size, x=x, y=y, anchor_x='center', anchor_y='center');
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
			x=window.width // 2,
			y=window.height - 100,
			on_press=lambda b: b.app.change_state('settings')),
		Button(
			"play",
			x=window.width // 2,
			y=window.height - 150,
			on_press=lambda b: b.app.change_state('game')),
		Button(
			"exit",
			x=window.width // 2,
			y=window.height - 200,
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
			x=window.width // 2,
			y=window.height - 100,
			on_press=lambda b: b.app.change_state('main')),
		]))
	game_menu = Popup('menu', [
		Button(
			"menu",
			x=window.width - 130,
			y=50,
			on_press=lambda b: b.screen.show_popup('menu')),
		])
	app.add_screen(Screen('game', [
		Button(
			"menu",
			x=window.width - 130,
			y=50,
			on_press=lambda b: b.screen.show_popup('menu')),
		]))
	app.add_screen(Screen('exit', [],
		exit_time=0.0,
		on_exit=lambda s: s.app.exit()
	))

init_screens(app)

@window.event
def on_draw():
	window.clear()
	app.draw()

@window.event
def on_mouse_press(x, y, buttons, modifiers):
	app.on_mouse_press(x, y, buttons, modifiers)

pyglet.clock.schedule_interval(app.update, 1.0/12.0)
pyglet.app.run()
