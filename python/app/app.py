import pyglet

class App():
	def __init__(self, initial, screens):
		self.screens = {}
		for screen in screens:
			screen.app = self
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

class Screen():
	def __init__(self, name, wigits, exit_time = None, exit_state = None):
		self.name = name
		self.wigits = wigits
		self.exit_time = exit_time
		self.exit_state = exit_state
		self.elapsed_time = 0.0
		self.app = None

	def on_enter(self):
		self.elapsed_time = 0.0

	def update(self, dt):
		self.elapsed_time += dt
		if self.exit_time and self.elapsed_time >= self.exit_time:
			app.change_state(self.exit_state)

	def draw(self):
		for wigit in self.wigits:
			wigit.draw()

window = pyglet.window.Window()

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
		], exit_time=2.0, exit_state="main"),
	Screen("main", [
		pyglet.text.Label(
			'BrushLink',
			font_size=36,
			x=window.width // 2,
			y=window.height - 40,
			anchor_x='center',
			anchor_y='center'),
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


pyglet.clock.schedule_interval(app.update, 1.0/12.0)
pyglet.app.run()