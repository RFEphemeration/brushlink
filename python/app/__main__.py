from app import *

window = pyglet.window.Window(resizable=True)

app = App()

app.update_window_size(window.get_size())

add_screens(app)

app.change_state('title')

@window.event
def on_draw():
	window.clear()
	app.draw()

@window.event
def on_mouse_press(x, y, buttons, modifiers):
	app.on_mouse_press(x, y, buttons, modifiers)

@window.event
def on_resize(width, height):
	app.update_window_size(window.get_size())

pyglet.clock.schedule_interval(app.update, 1.0/12.0)
pyglet.app.run()