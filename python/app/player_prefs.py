

class CommandCardPrefs():
	def __init__(self):
		self.columns = 3
		self.rows = 4

class PlayerPrefs():
	def __init__(self):
		self.exposed_elements = {
			'unit_type': ['spawner', 'healer', 'melee'],
			'unit_group': ['of_type', 'group_number'],
			'number': ['1', '2', '3', '4', '5', '6', '7', '8', '9', '0'],
			'ability': ['move', 'stop', 'attack', 'spawn', 'heal'],
			'action': ['select', 'use_ability', 'set_group'],
		}
		self.command_card = CommandCardPrefs()