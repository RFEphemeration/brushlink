

class CommandPrefs():
	def __init__(self):
		self.card_columns = 3
		self.card_rows = 4
		self.root_node = 'Some'
		self.modules = []
		self.exposed_elements = {
			'Meta': ['Execute', 'Skip', 'Cancel'],
			'Boolean': ['True', 'False', 'Compare'],
			'Comparison': ['Equal', 'NotEqual'],
			'Number': ['1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'Sum'],
		}
		"""
		self.exposed_elements = {
			'unit_type': ['spawner', 'healer', 'melee'],
			'unit_group': ['of_type', 'group_number'],
			'number': ['1', '2', '3', '4', '5', '6', '7', '8', '9', '0'],
			'ability': ['move', 'stop', 'attack', 'spawn', 'heal'],
			'action': ['select', 'use_ability', 'set_group'],
		}
		"""

class PlayerPrefs():
	def __init__(self):
		self.command = CommandPrefs()