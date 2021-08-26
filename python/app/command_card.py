from pyglet.window import key
from pprint import pprint
from burl.language import Context, EvaluationError, Value
from burl.parser import ParseNode


class CommandCard():
	# rmf todo: configurable keys, common keyboard layouts
	card_keys = [
		[key._1, key._2, key._3, key._4, key._5,],
		[key.Q, key.W, key.E, key.R, key.T,],
		[key.A, key.S, key.D, key.F, key.G,],
		[key.Z, key.X, key.C, key.V, key.B,],
	]

	def __init__(self, tab_indexes, tabs, tab_buttons, element_buttons, command_label, prefs):
		self.tab_indexes = tab_indexes
		self.tabs = tabs
		self.tab_buttons = tab_buttons
		self.element_buttons = element_buttons
		self.active_type = None
		self.command_label = command_label
		self.command_label.label.width = 100
		self.command_label.label.multiline = True
		self.prefs = prefs
		hotkey_index = 0
		self.hotkeys={}
		self.burl_context = Context()
		self.burl_context.parse_eval("LoadModule composition")
		for module in self.prefs.modules:
			self.burl_context.parse_eval("LoadModule " + module)
		self.burl_cursor = None

		for row in range(min(self.prefs.card_rows, len(CommandCard.card_keys))):
			for column in range(min(self.prefs.card_columns, len(CommandCard.card_keys[row]))):
				self.hotkeys[CommandCard.card_keys[row][column]] = hotkey_index
				hotkey_index += 1

		for element_type, index in self.tab_indexes.items():
			if index == 0:
				self.change_tab(element_type)
				break

		self.update_cursor()
		self.command_label.label.text = str(self.burl_cursor)
		self.command_label.label.font_size = self.command_label.label.font_size

	def change_tab(self, element_type):
		self.tabs.set_active_tab(self.tab_indexes[element_type])
		self.active_type = element_type
		# disable element_buttons that aren't allowed

	def append_element(self, element, element_type):
		self.element_buttons['Meta']['Skip'].enable()
		if element_type == 'Meta':
			if element == 'Execute':
				result = self.burl_context.parse_eval("Evaluate Cursor.GetEvalNode Get cursor")
				print(result)
				self.burl_cursor = None
			elif element == 'Skip':
				success = self.burl_cursor.value.increment_path_to_open_parameter(force_one=True)
				if not success:
					self.element_buttons['Meta']['Skip'].disable()
			elif element == 'Cancel':
				self.burl_cursor = None
		else:
			self.burl_context.parse_eval("Cursor.InsertArgument Get cursor Quote " + element)
		self.update_cursor()

		self.command_label.label.text = str(self.burl_cursor)
		self.command_label.label.font_size = self.command_label.label.font_size
		

	def on_key_press(self, symbol, modifiers):
		if modifiers:
			return False
		if symbol not in self.hotkeys:
			return False
		if self.active_type not in self.prefs.exposed_elements:
			return False
		index = self.hotkeys[symbol]
		if index >= len(self.prefs.exposed_elements[self.active_type]):
			return False
		element = self.prefs.exposed_elements[self.active_type][index]
		element_button = self.element_buttons[self.active_type][element]
		element_button.try_press()
		return True

	def update_cursor(self):
		if self.burl_cursor is None:
			self.burl_cursor = self.burl_context.parse_eval("Set cursor Cursor.Make Quote " + self.prefs.root_node)
		
		options = self.burl_context.parse_eval("Cursor.GetAllowedArgumentTypes Get cursor")
		if not options.value:
			# rmf todo: reset cursor
			return

		min_type = None
		allowed_types = {o.value for o in options.value}
		for element_type, type_button in self.tab_buttons.items():
			if element_type in allowed_types or element_type == 'Meta':
				type_button.enable()
				if element_type != 'Meta' and (min_type is None
						or self.tab_indexes[element_type] < self.tab_indexes[min_type]):
					min_type = element_type
			else:
				type_button.disable()
				
		if min_type is None:
			min_type = 'Meta'
		self.change_tab(min_type)
