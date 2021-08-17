from app import *

window = pyglet.window.Window(fullscreen=True)

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

pyglet.clock.schedule_interval(app.update, 1.0/12.0)
pyglet.app.run()