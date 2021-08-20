

class CommandCardPrefs():
	def __init__(self):
		self.columns = 4
		self.rows = 3

class PlayerPrefs():
	def __init__(self):
		self.exposed_elements = {
			'unit_type': ['spawner', 'healer', 'melee'],
			'unit_group': ['of_type', 'group_number'],
			'number': ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9'],
			'ability': ['move', 'stop', 'attack', 'spawn', 'heal'],
			'action': ['select', 'use_ability', 'set_group'],
		}
		self.command_card = CommandCardPrefs()