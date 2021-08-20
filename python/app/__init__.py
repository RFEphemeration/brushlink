from app.app import *


def _make_screen_title(app):
	return Screen('title', [
		Label('BrushLink',
			font_size=(36,'px'),
			box=Box(over=(0,"px"), up=(60,"px"), width=(0,"px"),height=(36, "px"))),
		Label('by ephemeration games',
			font_size=(24,'px'),
			box=Box(over=(0,"px"), up=(10,"px"), width=(0,"px"),height=(24, "px"))),
		],
		exit_time=1.5,
		on_exit=lambda s: app.change_state('main')
	)


def _make_screen_main(app):
	return Screen('main', [
		Label('BrushLink',
			font_size=(36,'px'),
			box=Box(over=(0,"px"), top=(40,"px"), width=(0,"px"),height=(36, "px"))),
		Button(
			"settings",
			box=Box(over=(0,"px"),up=(100,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: app.change_state('settings')),
		Button(
			"play",
			box=Box(over=(0,"px"),up=(50,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: app.change_state('game')),
		Button(
			"exit",
			box=Box(over=(0,"px"),up=(0,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: app.change_state('exit')),
		]
	)


def _make_screen_settings(app):
	return Screen('settings', [
		Label('Settings',
			font_size=(36,'px'),
			box=Box(over=(0,"px"), top=(40,"px"), width=(0,"px"),height=(36, "px"))),
		Button(
			'back',
			box=Box(over=(0,"px"),up=(100,"px"),width=(200,"px"),height=(40,"px")),
			on_press=lambda b: app.change_state('main')),
		]
	)


def _make_screen_game(app):
	game_menu = Panel(name='menu', enabled=False,
		box=Box(over=(0,"px"),up=(0,"px"),width=(40,"vw"),height=(30,"vw")),
		wigits=[
			Button(
				"resume",
				box=Box(bottom=(1,"vw"),right=(1,"vw"),width=(15,"vw"),height=(4,"vw")),
				on_press=lambda b: game_menu.hide()),
			Button(
				"leave",
				box=Box(bottom=(1,"vw"),left=(1,"vw"),width=(15,"vw"),height=(4,"vw")),
				on_press=lambda b: app.change_state('main')),
			])

	element_panels = {}
	type_buttons = {}
	element_buttons = {}
	player_prefs = app.get_player_prefs()
	type_height_ratio = 1.0 / len(player_prefs.exposed_elements)
	type_index = 0
	element_cols = player_prefs.command_card.columns
	element_width_ratio = 1.0 / player_prefs.command_card.columns
	element_height_ratio = 1.0 / player_prefs.command_card.rows
	element_types = []
	command_card = None
	game_screen = None
	tab_indexes = {}
	for element_type, elements in player_prefs.exposed_elements.items():
		tab_indexes[element_type] = type_index
		element_types.append(element_type)
		element_buttons[element_type] = {}
		panel_buttons = []
		element_index = 0
		for element in elements:
			element_buttons[element_type][element] = Button(element,
				on_press=lambda b, e=element: game_screen.command_append_element(e),
				box=Box(
					top=((element_index // element_cols) * element_height_ratio * 60, 'ph'),
					left=((element_index % element_cols) * element_width_ratio * 60, 'pw'),
					width=(element_width_ratio * 60, 'pw'),
					height=(element_height_ratio * 60, 'ph')))
			panel_buttons.append(element_buttons[element_type][element])
			element_index += 1


		type_buttons[element_type] = Button(element_type,
			on_press=lambda b, t=element_type: game_screen.command_change_tab(t),
			box=Box(
				top=(type_height_ratio * 60 * type_index, 'ph'),
				left=(0,'pw'),
				width=(6, 'vw'),
				height=(type_height_ratio * 60, 'ph')))
		element_panels[element_type] = Panel(
			box=Box.Fill(),
			wigits=panel_buttons)
		type_index += 1


	command_card = Tabs(box=Box(bottom=(1,'vw'),right=(1,'vw'),width=(20,'vw'),height=(20,'vw')),
		contents=[element_panels[t] for t in element_types])
	card_tabs = Panel(
		box=Box(bottom=(1,'vw'),right=(22,'vw'),width=(6,'vw'),height=(20,'vw')),
		wigits=[type_buttons[t] for t in element_types])

	game_screen = MatchScreen('game',
		wigits=[
			Button(
				"menu",
				box=Box(bottom=(1,"vw"),right=(1,"vw"),width=(15,"vw"),height=(4,"vw")),
				on_press=lambda b: game_menu.show()),
			command_card,
			card_tabs,
			game_menu,
		],
		command_card=command_card,
		command_tab_indexes=tab_indexes
	)

	return game_screen


def _make_screen_exit(app):
	return Screen('exit', [],
		exit_time=0.0,
		on_exit=lambda s: app.exit()
	)


def _make_screen_layout_test(app):
	return Screen('layout_test', [
		Panel(
			box=Box(top=(10,"px"),left=(10,"px"),bottom=(10,"px"), right=(10,"px")),
			wigits = [
				Panel([], color=(100,50,50),
					box=Box(over=(0,"px"),up=(0,"px"),height=(200,"px"), width=(200,"px")))
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


"""
Panel style:{ top:0, bottom:0, left:0, right:0 }
	Panel style:{ over:0, up:0, width:40vw, height:min(30vw,50vh) }
		Button style:{  }
			text: 'settings'
"""
