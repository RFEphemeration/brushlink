from app import *

window = pyglet.window.Window(resizable=True)
window.set_minimum_size(480, 360)

app = App(window)
add_screens(app)
app.change_state('title')

pyglet.clock.schedule_interval(app.update, 1.0/12.0)
pyglet.app.run()