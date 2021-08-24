from app import *

window = pyglet.window.Window(resizable=True)
window.set_minimum_size(480, 360)

App.instance = App(window)
add_screens(App.instance)

App.instance.change_state('title')

pyglet.clock.schedule_interval(App.instance.update, 1.0/12.0)
pyglet.app.run()