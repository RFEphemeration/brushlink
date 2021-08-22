from app.ui import *
from pyglet.window import key
from pprint import pprint


class CommandCard():
	# rmf todo: configurable keys, common keyboard layouts
	card_keys = [
		[key._1, key._2, key._3, key._4, key._5,],
		[key.Q, key.W, key.E, key.R, key.T,],
		[key.A, key.S, key.D, key.F, key.G,],
		[key.Z, key.X, key.C, key.V, key.B,],
	]

	def __init__(self, tab_indexes, tabs, tab_buttons, element_buttons, command_label, card_dimensions, exposed_elements):
		self.tab_indexes = tab_indexes
		self.tabs = tabs
		self.tab_buttons = tab_buttons
		self.element_buttons = element_buttons
		self.active_type = None
		self.command_label = command_label
		self.card_dimensions = card_dimensions
		self.exposed_elements = exposed_elements
		hotkey_index = 0
		self.hotkeys={}
		for row in range(min(self.card_dimensions[0], len(CommandCard.card_keys))):
			for column in range(min(self.card_dimensions[1], len(CommandCard.card_keys[row]))):
				self.hotkeys[CommandCard.card_keys[row][column]] = hotkey_index
				hotkey_index += 1

		for element_type, index in tab_indexes.items():
			if index == 0:
				self.change_tab(element_type)
				break

		self.previous_append = None

	def change_tab(self, element_type):
		self.tabs.set_active_tab(self.tab_indexes[element_type])
		self.active_type = element_type
		# disable element_buttons that aren't allowed

	def append_element(self, element, element_type):
		self.command_label.label.multiline = True
		self.command_label.label.text = self.command_label.label.text + "\n" + element
		self.command_label.label.font_size = self.command_label.label.font_size
		if self.previous_append:
			self.element_buttons[self.previous_append[0]][self.previous_append[1]].enable()

		self.element_buttons[element_type][element].disable()
		self.previous_append = (element_type, element)
		# disable type buttons that aren't allowed

	def on_key_press(self, symbol, modifiers):
		if modifiers:
			return False
		if symbol not in self.hotkeys:
			return False
		if self.active_type not in self.exposed_elements:
			return False
		index = self.hotkeys[symbol]
		if index >= len(self.exposed_elements[self.active_type]):
			return False
		element = self.exposed_elements[self.active_type][index]
		element_button = self.element_buttons[self.active_type][element]
		element_button.try_press()
		return True
