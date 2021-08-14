from collections import defaultdict, namedtuple
from enum import Enum
from functools import lru_cache

class ActionSettings:
	ActionTypes = ["Idle", "Complete", "Move", "Attack", "Heal", "Reproduce"]
	def __init__(self, action_type, cost, magnitude, duration, cooldown):
		self.action_type = action_type
		self.cost = cost
		self.magnitude = magnitude
		self.duration = duration
		self.cooldown = cooldown
		# todo: target_types, valid offsets


class ActionStep:
	def __init__(self, action_type, target_offset = None, target_unit = None):
		self.action_type = action_type
		self.target_offset = target_offset
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
		self.evaluator = ParseNode.parse("""EvaluateElement $command $unit_id $player_id""").to_eval_node(self.burl_context, builtin_nodes={
				"$command": EvalNode(Literal(None, "NoneType")),
				"$unit_id": EvalNode(Literal(unit_id, "UnitID")),
				"$player_id": EvalNode(Literal(player_id, "PlayerID"))
			})

	@property
	def x(self):
		return self.position[0]

	@property
	def y(self):
		return self.position[1]
	

	def update_pending(self):
		while self.command_queue and (self.pending is None or self.pending.action_type == "Complete"):
			self.command_queue = self.command_queue[1:]
			# todo: pass in values like the unit_id and such, if this is an element
			# or set globals in our context with our id
			self.pending = self.command_queue[0].evaluate(self.burl_context).value

		if not self.command_queue:
			self.pending = None
		# todo: sanitize action steps so that units don't break the rules
		return self.pending or ActionStep("Idle")

class Color(Enum):
	GREEN_CHECKER = 0
	PURPLE_STRIPE = 1


Position = namedtuple('Position', ('x', 'y'))

class Player:
	GREEN_CHECKER = 0
	PURPLE_STRIPE = 1
	COLORS = [GREEN_CHECKER, PURPLE_STRIPE]
	def __init__(self, player_id, spawn_location, color, board_bounds):
		self.player_id = player_id
		self.camera_center = spawn_location
		self.color = color
		self.vision_grid = [[0 for x in range(board_bounds[1][1])] for x in range(board_bounds[1][0])]

	@staticmethod
	@lru_cache(5)
	def visible_positions(board_bounds, camera_bounds):
		positions = []

		for x in range(camera_bounds[0][0], camera_bounds[1][0] + 1, 1):
			if x < board_bounds[0][0] or x > board_bounds[1][0]:
				continue
			for y in range(camera_bounds[0][1], camera_bounds[1][1] + 1, 1):
				if y < board_bounds[0][1] or y > board_bounds[1][1]:
					continue
				positions.append(Position(x, y));
		return positions

class Tile(Enum):
	Void = 0
	Ground = 1


class Board:
	TileTypes = ["Void", "Ground"]

	def __init__(self, area, spawns):
		self.area = area
		self.spawns = spawns
		self.size = Position(len(area), len(area[0]))

	def is_pathable(self, position):
		return position[0] >= 0 and position[0] < self.size[0] and \
			position[1] >= 0 and position[1] < self.size[0] and \
			self.area[position[0]][position[1]] == Tile.Ground


class Match:
	def __init__(self, board, players, unit_data, starting_units, update_dt):
		self.board = board
		self.players = players
		self.unit_data = unit_data
		self.update_dt = update_dt
		self.starting_units = starting_units
		self.next_unit_id = 0
		self.update_count = 0
		self.units = {}
		self.deleted_units = []
		self.positions = {}
		self.seconds_to_ticks = {}
		for unit_type in self.unit_data:
			recharge_seconds = self.unit_data[unit_type].recharge_rate[1]
			self.seconds_to_ticks[recharge_seconds] = round(recharge_seconds / update_dt)

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
		self.deleted_units.clear()
		self.update_count += 1
		action_phases = {}
		# todo order-independent movement and stat updates
		for unit_id in self.units:
			unit = self.units[unit_id]
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
					if action_step.target_position not in self.positions \
					and action_step.target_position not in new_positions:
						new_positions[action_step.target_position] = unit.unit_id
						took_action = True
				elif action_type == "Reproduce":
					pass

				if took_action:
					# todo: cooldowns
					energy_deltas[unit.unit_id] -= action_settings.cost

		for unit_id in self.units:
			unit = self.units[unit_id]
			# performance: consider lifting this outside of the loop, or keept different sets for different ticks
			if self.update_count % self.seconds_to_ticks[unit.settings.recharge_rate[1]]:
				energy_deltas += unit.settings.recharge_rate[0]

		for unit_id in energy_deltas:
			delta = energy_deltas[unit_id]
			unit = self.units[unit_id]
			unit.energy = max(0, min(unit.energy + delta, unit.settings.max_energy))
			




