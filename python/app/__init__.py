from app.app import *


def _make_screen_title(app):
	return Screen('title', [
		pyglet.text.Label(
			'BrushLink',
			font_size=36,
			x=app.window_size.width // 2,
			y=app.window_size.height // 2 + 40,
			anchor_x='center',
			anchor_y='center'),
		pyglet.text.Label(
			'by ephemeration games',
			font_size=24,
			x=app.window_size.width // 2,
			y=app.window_size.height // 2 - 20,
			anchor_x='center',
			anchor_y='center'),
		],
		exit_time=0.5,
		on_exit=lambda s: app.change_state('main')
	)


def _make_screen_main(app):
	return Screen('main', [
		pyglet.text.Label(
			'BrushLink',
			font_size=36,
			x=app.window_size.width // 2,
			y=app.window_size.height - 40,
			anchor_x='center',
			anchor_y='center'),
		Button(
			"settings",
			box=Box(hor=(0,"px"),vert=(100,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: app.change_state('settings')),
		Button(
			"play",
			box=Box(hor=(0,"px"),vert=(50,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: app.change_state('game')),
		Button(
			"exit",
			box=Box(hor=(0,"px"),vert=(0,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: app.change_state('exit')),
		]
	)


def _make_screen_settings(app):
	return Screen('settings', [
		pyglet.text.Label(
			"Settings",
			font_size=36,
			x=app.window_size.width // 2,
			y=app.window_size.height - 40,
			anchor_x='center',
			anchor_y='center'),
		Button(
			'back',
			box=Box(hor=(0,"px"),vert=(100,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: app.change_state('main')),
		]
	)


def _make_screen_game(app):
	game_menu = Panel(name='menu', enabled=False, wigits=[
		Button(
			"resume",
			box=Box(bottom=(10,"px"),right=(10,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: game_menu.hide()),
		Button(
			"leave",
			box=Box(bottom=(10,"px"),left=(10,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: app.change_state('main')),
		],
		box=Box(hor=(0,"px"),vert=(0,"px"),width=(430,"px"),height=(300,"px")))
	return Screen('game', on_enter=lambda s: game_menu.hide(), wigits=[
		game_menu,
		Button(
			"menu",
			box=Box(bottom=(20,"px"),right=(20,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: game_menu.show()),
		]
	)


def _make_screen_exit(app):
	return Screen('exit', [],
		exit_time=0.0,
		on_exit=lambda s: s.app.exit()
	)


def _make_screen_layout_test(app):
	return Screen('layout_test', [
		Panel(
			box=Box(top=(10,"px"),left=(10,"px"),bottom=(10,"px"), right=(10,"px")),
			wigits = [
				Panel([], color=(100,50,50),
					box=Box(hor=(0,"px"),vert=(0,"px"),height=(200,"px"), width=(200,"px")))
			])
		]
	)


def add_screens(app):
	app.add_screen('layout_test', _make_screen_layout_test)
	app.add_screen('title', _make_screen_title)
	app.add_screen('main', _make_screen_main)
	app.add_screen('settings', _make_screen_settings)
	app.add_screen('game', _make_screen_game)
	app.add_screen('exit', _make_screen_exit)
