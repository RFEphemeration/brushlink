from collections import defaultdict


class Board:
	def __init__(self, area, spawns):
		self.area = area
		self.spawns = spawns

class ActionSettings:
	def __init__(self, cost, magnitude, duration, cooldown):
		self.cost = cost
		self.magnitude = magnitude
		self.duration = duration
		self.cooldown = cooldown

class ActionStep:
	# Idle, Complete, Move, Attack, Heal, Reproduce
	def __init__(self, action_type, target_position = None, target_unit = None):
		self.action_type = action_type
		self.target_position = target_position
		self.target_unit = target_unit


class UnitSettings:
	def __init__(self, unit_type, starting_energy = 6, max_energy = 12, recharge_rate = (1, 1.0), graphics = None, actions = None, vision_radius = 4.5, targeted_modifiers = None):
		self.unit_type = unit_type
		self.starting_energy = starting_energy
		self.max_energy = max_energy
		self.recharge_rate = recharge_rate
		self.graphics = graphics
		self.actions = actions or {}
		self.vision_radius = vision_radius
		self.targeted_modifiers = targeted_modifiers or {}


class Unit:
	def __init__(self, settings, unit_id, player_id, position):
		self.settings = settings
		self.unit_id = unit_id
		self.player_id = player_id
		self.position = position
		self.energy = self.settings.starting_energy
		self.crowded_duration = 0
		self.pending = None
		self.command_queue = []
		self.burl_context = Context() # todo: make this reference player's context as parent

	def update_pending(self):
		while self.command_queue and (self.pending is None or self.pending.action_type == "Complete"):
			self.command_queue = self.command_queue[1:]
			# todo: pass in values like the unit_id and such, if this is an element
			# or set globals in our context with our id
			self.pending = self.command_queue[0].evaluate(self.burl_context).value

		if not self.command_queue:
			self.pending = None
		# todo: sanitize action steps so that units don't break the rules
		return self.pending or return ActionStep("Idle")


class Match:
	def __init__(self, board, players, unit_data, starting_units, update_rate):
		self.board = board
		self.players = players
		self.unit_data = unit_data
		self.update_rate = update_rate
		self.starting_units = starting_units
		self.next_unit_id = 0
		self.update_count = 0
		self.units = {}
		self.positions = {}

	def setup(self):
		for player, spawn in zip(self.players, self.board.spanws):
			for (offset, unit_type) in starting_units:
				spawn_unit(unit_type, player.player_id, spawn + offset)

	def spawn_unit(self, unit_type, player_id, position):
		if position in self.positions:
			raise GameError("Can't spawn unit at " + position.__str__() + " because it is already occupied")
		unit = Unit(self.unit_data[unit_type], self.next_unit_id, player_id, position)
		self.units[unit.unit_id] = unit
		self.positions[position] = unit.unit_id
		self.next_unit_id += 1

	def update(self):
		self.update_count += 1
		action_phases = {}
		# todo order-independent movement and stat updates
		for unit in self.units:
			while not unit.pending_action and unit.command_queue:
				action_step = unit.update_pending()
				if action_step:
					if action_step.action_type not in action_phases:
						action_phases[action_step.action_type] = []
					action_phases[action_step.action_type].append((action_step, unit))

		energy_deltas = defaultdict(int)
		new_positions = {}
		for action_type in ["Heal", "Reproduce", "Attack", "Move"]:
			for (action_step, unit) in action_phases[action_type]:
				action_settings = unit.actions[action_step.action_type]
				# todo: also check cooldown
				if unit.energy < action_settings.cost:
					# can't perform this action
					# should we give feedback to unit about what happened?
					continue
				took_action = False
				if action_type == "Attack":
					energy_deltas[action_step.target_unit] -= action_settings.magnitude
					took_action = True
				elif action_type == "Heal":
					energy_deltas[action_step.target_unit] += action_settings.magnitude
					took_action = True
				elif action_type == "Move":
					# how to handle priority here for trying to move to the same place? favor older units?
					if action_step.target_position not in self.positions and not in new_positions
						new_positions[action_step.target_position] = unit.unit_id
						took_action = True
				elif action_type == "Reproduce":
					pass

				if took_action:
					# todo: cooldowns
					energy_deltas[unit.unit_id] -= action_settings.cost

			del action_phases[phase]


