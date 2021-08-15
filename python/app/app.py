import os
import pyglet
from game import Match

pyglet.resource.path = [os.path.dirname(__file__) + '/resources/']
pyglet.resource.reindex()

class App():
	def __init__(self, initial, screens):
		self.screens = {}
		for screen in screens:
			screen.set_app(self);
			self.screens[screen.name] = screen
		self.elapsed_time = 0.0
		self.change_state(initial)

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

class Screen():
	def __init__(self, name, wigits, exit_time = None, exit_state = None):
		self.name = name
		self.wigits = wigits
		self.exit_time = exit_time
		self.exit_state = exit_state
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
		if self.exit_time and self.elapsed_time >= self.exit_time:
			app.change_state(self.exit_state)

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


class Button(pyglet.gui.WidgetBase):
	def __init__(self, label, x, y, on_press,font_size = 18, color=(155,155,255), width=200, height=40):
		super().__init__(x - width // 2, y - height // 2, width, height);
		self.background = pyglet.shapes.Rectangle(x=x, y=y, width=width, height=height, color=color)
		self.background.anchor_x = width // 2;
		self.background.anchor_y = height // 2;
		self.label = pyglet.text.Label(label, font_size, x=x, y=y, anchor_x='center', anchor_y='center');
		'''
		depressed = pyglet.resource.image('button_up.png')
		pressed = pyglet.resource.image('button_down.png')

		texture = pyglet.image.Texture.create(width, height, rectangle=True)
		self.button = pyglet.gui.PushButton(x, y, pressed=texture, depressed=texture)
		self.button.set_handler('on_press', lambda: on_press(self))
		'''
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

app = App(initial="title", screens=[
	Screen("title", [
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
		], exit_time=0.5, exit_state="main"),
	Screen("main", [
		pyglet.text.Label(
			'BrushLink',
			font_size=36,
			x=window.width // 2,
			y=window.height - 40,
			anchor_x='center',
			anchor_y='center'),
		Button(
			'settings',
			x=window.width // 2,
			y=window.height - 100,
			on_press=lambda b: b.app.change_state("settings")),
		]),
	Screen("settings", [
		pyglet.text.Label(
			'Settings',
			font_size=36,
			x=window.width // 2,
			y=window.height - 40,
			anchor_x='center',
			anchor_y='center'),
		]),
	])


@window.event
def on_draw():
	window.clear()
	app.draw()

@window.event
def on_mouse_press(x, y, buttons, modifiers):
	app.on_mouse_press(x, y, buttons, modifiers)

pyglet.clock.schedule_interval(app.update, 1.0/12.0)
pyglet.app.run()