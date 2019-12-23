from collections import namedtuple
import random

wallet_initial = 100.0
total_unit_cost = 500.0
total_units = 50
total_players = 1000

min_unit_cost = 1.0
max_starting_unit_cost = 35.0
max_unit_valuation = 75.0

# could have unit archetypes for how players choose them
# true rating understood by everyone of either high value or low value
# divisive unit with some people thinking high value and others low value
# with some ratio between the two groups of people
# true rating, percentage underestimating, correct, over

archetypes = {
	"Basic": [.1, .8, .1],
	"Divisive": [.4, .1, .4],
	"Overestimated": [.1, .5, .4],
	"Underestimated": [.4, .5, .1]
}

class Unit:
	def __init__(self, id, archetype):
		self.id = id
		self.archetype = archetype
		self.value = random.gauss(10.0, 5.0)
		self.cost = random.randrange(min_unit_cost, max_starting_unit_cost)
		if (self.value < min_unit_cost):
			self.value = min_unit_cost
		self.uses = 0
		self.cost_history = []
		self.usage_rate_history = []


units = [ Unit(id=id) for id in range(total_units)]


def normalize_unit_costs():
	for unit in units:
		if unit.cost < min_unit_cost:
			unit.cost = min_unit_cost
	total_unit_cost_actual = sum([unit.cost for unit in units])
	ratio = total_unit_cost_actual / total_unit_cost
	extra_to_remove = 0
	if ratio > 0.999 and ratio < 1.001:
		return
	for unit in units:
		new_cost = unit.cost * ratio
		if new_cost < min_unit_cost:
			extra_to_remove = min_unit_cost - new_cost 
			new_cost = min_unit_cost
		unit.cost = new_cost
	while extra_to_remove > min_unit_cost:
		unit = random.choice(units)
		if unit.cost > min_unit_cost:
			new_cost = unit.cost - min_unit_cost
			if new_cost < min_unit_cost:
				new_cost = min_unit_cost
			extra_to_remove -= unit.cost - new_cost
			unit.cost = new_cost


def calculate_unit_costs():
	total_unit_uses = 0
	for unit in units:
		unit.cost_history.append(unit.cost)
		unit.usage_rate_history.append(unit.uses)
		total_unit_uses += unit.uses

		# this is what percentage of all units picked were this unit
		ratio_of_units_picked = unit.uses / total_unit_uses
		# should we instead be using what percentage of players picked this unit?
		ratio_of_player_picks = unit.uses / total_players

		# this ensures the sum is equal to the total_unit_cost
		new_cost = ratio_of_units_picked * total_unit_cost
		# but if they weren't picked because their previous cost was too high
		# their new_cost might be way too low?

		unit.cost = new_cost


class Player:
	def __init__(self, id):
		self.id = id
		self.wallet = wallet_initial
		self.unit_set = []
		self.unit_set_history = []
		self.unit_valuations = []
		for unit in units:
			valuation = unit.value
			archetype = archetypes[unit.archetype]
			roll = random.randrange(1.0)
			if roll < archetype[0]:
				# this player underestimates the unit
				valuation = random.randrange(0.1, 0.8) * unit.value
				if (valuation < min_unit_cost):
					valuation = min_unit_cost
			elif roll < archetype[1] + archetype[0]:
				# this player correctly evaluates the unit
				valuation = random.randrange(0.9,1.1) * unit.value
			else:
				# this player overestimates the unit
				valuation = random.randrange(1.5, 2.5) * unit.value
				if (valuation > max_unit_valuation):
					valuation = max_unit_valuation
			unit_valuations.append(valuation)


players = [ Player(id=id) for id in range(total_players)]


normalize_unit_costs()
for season in range(100):
	for unit in units:
		unit.uses = 0
	for player in players:
		player.wallet = wallet_initial
		player.unit_set = []
		unit_choices = [unit.id for unit in units]
		# this is where we would put actual player choice if we had it
		# maybe ordering by difference between cost and player's valuation
		# but it's still fine to pick greedily
		random.shuffle(unit_choices)
		for unit_id in unit_choices:
			unit = units[unit_id]
			if (unit.cost > player.wallet):
				continue
			player.wallet -= unit.cost
			player.unit_set.append(unit_id)

			units[unit_id].uses++

		player.unit_set_history.append(player.unit_set)


