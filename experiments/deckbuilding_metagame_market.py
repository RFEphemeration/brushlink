from __future__ import division
from collections import namedtuple
import random
import copy

import plotly.express as px
import plotly.io as pio
import pandas as pd

wallet_initial = 100.0
total_unit_cost = 500.0
total_units = 50
total_players = 1000
max_team_size = 15

seasons = 15
seasons_to_print = 15

min_unit_cost = 1.0
average_unit_cost = total_unit_cost / total_units

average_team_size = wallet_initial / average_unit_cost

max_starting_unit_cost = average_unit_cost * 3.5
max_unit_valuation = wallet_initial * .8

adjustment_seasons = 3
starting_lerp_value = 0.15
target_lerp_value = 0.15

adjustment_method = "b"

#log = open("log.txt","w+")

def debug_print(value):
	#log.write(str(value) + "\n")
	#print(value)
	pass


def lerp(a, b, t):
	return (a * (1.0 - t)) + (b * t)


class Unit:
	
	# could have unit archetypes for how players choose them
	# divisive unit with some people thinking high value and others low value
	# true rating understood by everyone of either high value or low value
	# with some ratio between the two groups of people
	# true rating, percentage underestimating, correct, over
	archetypes = {
		"Basic": (.1, .8, .1),
		"Divisive": (.4, .1, .4),
		"Overestimated": (.1, .5, .4),
		"Underestimated": (.4, .5, .1)
	}

	def __init__(self, id):
		self.id = id
		# archetype testing should come after testing player valuation
		self.archetype = random.choice(list(Unit.archetypes.keys()))
		self.value = random.gauss(average_unit_cost + min_unit_cost, average_unit_cost )
		cost = random.uniform(min_unit_cost, max_starting_unit_cost)
		cost = lerp(cost, 10.0, 0.6)
		if (self.value < min_unit_cost):
			self.value = min_unit_cost
		self.costs = [cost]
		self.uses = []
		self.target_costs = []

	@staticmethod
	def normalize_unit_costs(units):
		for unit in units:
			if unit.costs[-1] < min_unit_cost:
				unit.costs[-1] = min_unit_cost
		total_unit_cost_actual = sum([unit.costs[-1] for unit in units])
		ratio = total_unit_cost / total_unit_cost_actual
		extra_to_remove = 0.0
		if ratio > 0.999 and ratio < 1.001:
			return
		for unit in units:
			new_cost = unit.costs[-1] * ratio
			if new_cost < min_unit_cost:
				extra_to_remove = min_unit_cost - new_cost
				new_cost = min_unit_cost
			debug_print("%.1f -> %.1f" %(unit.costs[-1], new_cost))
			unit.costs[-1] = new_cost

		# this might be a problem portion. reducing min_unit_cost is probably in order
		# and is probably not important for the simulation purposes anyways?
		'''
		while extra_to_remove > min_unit_cost:
			unit = random.choice(units)
			if unit.cost > min_unit_cost:
				new_cost = unit.cost - min_unit_cost
				if new_cost < min_unit_cost:
					new_cost = min_unit_cost
				extra_to_remove -= unit.cost - new_cost
				unit.cost = new_cost
		'''

	@staticmethod
	def normalize_target_costs(units):
		total_target = sum([unit.target_costs[-1] for unit in units])
		ratio = total_unit_cost / total_target
		for unit in units:
			unit.target_costs[-1] = unit.target_costs[-1] * ratio

	"""
	@staticmethod
	def new_cost(cost, uses):
		'''
		This function returns an exaggerated (not exact) difference in cost
		Cost is stable when the adjustment_force is zero
		But this is also subject to the normalization of costs
		Because the total number of units used might not be consistent
		'''
		# this ratio could instead be uses / total_uses * average_team_size
		# but is it an actual average team size or the expected one?
		ratio_of_picked = uses / total_players
		purchasing_power = total_unit_cost / wallet_initial
		exaggerated_cost_estimation = (
			ratio_of_picked
			* purchasing_power
			* average_unit_cost )

		return exaggerated_cost_estimation - cost
	"""

	@staticmethod
	def calculate_target_costs(units, season):
		total_unit_uses = 0
		for unit in units:
			total_unit_uses += unit.uses[-1]

		debug_print (total_unit_cost)
		debug_print (total_unit_uses)

		for unit in units:
			# this is what percentage of all units picked were this unit
			ratio_of_units_picked = unit.uses[-1] / total_unit_uses
			# should we instead be using what percentage of players picked this unit?
			ratio_of_players_picked = unit.uses[-1] / total_players

			# this ensures the sum is equal to the total_unit_cost
			new_cost_a = ratio_of_units_picked * total_unit_cost
			# but if they weren't picked because their previous cost was too high
			# their new_cost might be way too low?

			# this isn't going to sum to the correct level, is it?
			# but I guess that's what normalization is for
			new_cost_b = ratio_of_players_picked \
				* total_unit_cost \
				* total_unit_cost \
				/ wallet_initial \
				/ total_units \

			new_cost_c = 0.5 + unit.uses[-1] / 16

			# does averaging help out at all? is this just a crutch or necessary?
			

			# if we're just using uses at the previous cost, and the previous cost
			# it feels like we aren't quite using enough information
			# what about previous uses at the cost before that? then we can compare
			# our target new_cost with that trajectory.
			# Are we swinging wildly back and forth? then lerping less is in order.
			# Are we still moving in the same direction? maybe lerp more

			# strategy is going to be bisecting based on previous few points,
			# can apply the secant method to try and bisect more accurately

			# other terms to look up, kalman filter, classical control theory, data assimilation

			debug_print("a: %.1f b: %.1f" % (new_cost_a, new_cost_b))
			target_cost = {
				"a": new_cost_a,
				"b": new_cost_b,
				"c": new_cost_c
				}
			unit.target_costs.append(target_cost[adjustment_method])

	@staticmethod
	def apply_target_costs_as_new_cost(units, season):
		lerp_value = target_lerp_value
		if season < adjustment_seasons:
			lerp_value = lerp(
				starting_lerp_value,
				target_lerp_value,
				season/adjustment_seasons)

		for unit in units:
			new_cost = lerp(unit.costs[-1], unit.target_costs[-1], lerp_value)

			debug_print ("calculated " + str(unit.uses[-1]) + ": " + str(unit.costs[-1]) + " -> " + str(new_cost))

			unit.costs.append(new_cost)


class Player:
	def __init__(self, id, units):
		self.id = id
		self.wallet = wallet_initial
		self.unit_set = []
		self.unit_set_history = []
		self.unit_valuations = []
		for unit in units:
			valuation = unit.value
			archetype = Unit.archetypes[unit.archetype]
			roll = random.random()
			if roll < archetype[0]:
				# this player underestimates the unit
				valuation = random.uniform(0.1, 0.8) * unit.value
				if (valuation < min_unit_cost):
					valuation = min_unit_cost
			elif roll < archetype[1] + archetype[0]:
				# this player correctly evaluates the unit
				valuation = random.uniform(0.9,1.1) * unit.value
			else:
				# this player overestimates the unit
				valuation = random.uniform(1.5, 2.5) * unit.value
				if (valuation > max_unit_valuation):
					valuation = max_unit_valuation
			self.unit_valuations.append(valuation)

	def pick_unit_set(self, units):
		self.wallet = wallet_initial
		self.unit_set = []
		unit_choices = [unit.id for unit in units]
		# this is where we would put actual player choice if we had it
		# maybe ordering by difference between cost and player's valuation
		# but it's still fine to pick greedily
		#random.shuffle(unit_choices)
		unit_choices = sorted(unit_choices, key=(lambda id: self.unit_valuations[id] - units[id].costs[-1]), reverse=True)

		for unit_id in unit_choices:
			unit = units[unit_id]
			if (unit.costs[-1] > self.wallet):
				continue
			self.wallet -= unit.costs[-1]
			self.unit_set.append(unit_id)

			units[unit_id].uses[-1] += 1
			if len(self.unit_set) > max_team_size:
				break

		self.unit_set_history.append(self.unit_set)




class Simulation:
	def __init__(self, units = None, players = None, adjustment_method_override = None):
		# messy, but okay for now
		global adjustment_method
		
		self.units = units or [ Unit(id=id) for id in range(total_units)]
		self.players = players or [ Player(id=id, units=self.units) for id in range(total_players)]
		self.total_unit_value = sum([unit.value for unit in self.units])
		self.adjustments_normalization_factor = []
		
		adjustment_method = adjustment_method_override or adjustment_method
		debug_print("adjustment " + adjustment_method)

	def run(self):
		Unit.normalize_unit_costs(self.units)
		for season in range(seasons):
			for unit in self.units:
				unit.uses.append (0)
			for player in self.players:
				player.pick_unit_set(self.units)
			for unit in self.units:
				debug_print(unit.uses[-1])
			Unit.calculate_target_costs(self.units, season)
			Unit.normalize_target_costs(self.units)
			Unit.apply_target_costs_as_new_cost(self.units, season)
			Unit.normalize_unit_costs(self.units)
		Unit.calculate_target_costs(self.units, season)
		Unit.normalize_target_costs(self.units)

	def calculate_adjustment_force_normalization(self):
		pass

	def log(self):
		print("Total Value: " + str(self.total_unit_value))
		row = "Value\tArchetype\tFinal\tUses\t"
		for season in range(seasons_to_print):
			row += ("%d" % season) + "\t"
		print(row)
		for unit in self.units:
			row = "%.1f\t%s\t%.1f\t%d\t" % (
				unit.value,
				unit.archetype.ljust(14, ' '),
				unit.costs[-1],
				sum(unit.uses))
			for season in range(seasons_to_print):
				row += ("%.1f" % unit.costs[season]).rjust(4, ' ') + "\t"
			print(row)

	def output_equilibrium(self):
		equilibrium = open("equilibrium.txt","a+")
		for unit in self.units:
			equilibrium.write("%d, %.1f\n" % (unit.uses[-1], unit.costs[-1]))

		# uses = 15.660800973947 * cost - 6.8566881375178
		# individuals
		# cost = 0.37586235660666 + 0.062297033674562 * uses
		# y=0.32292235745492+0.059622259717538x
		# y=0.61527940764027+0.062666670620476x

		# average
		# y=0.53678797505211+0.062229490681948x

		# cost to usage rate is pretty much linear
		# because I wrote it that way
		# cost = 0.5 + uses / 16 in equilibrium



	def graph(self, file_name):
		global adjustment_method

		file_name = file_name or "./market_output.html"

		data = {
			"season": [],
			"cost": [],
			"unit_id": [],
			"value": [],
			"type": [],
			"uses": [],
			"force": []
		}

		def append_data(unit_id, season, cost, uses):
			unit = self.units[unit_id]
			data["season"].append(season)
			data["cost"].append(cost)
			data["unit_id"].append(unit_id)
			data["value"].append(unit.value)
			data["type"].append(unit.archetype)
			data["uses"].append(uses)
			data["force"].append(
				unit.target_costs[season]
				- unit.costs[season])

		for unit_id in range(total_units):
			unit = self.units[unit_id]
			for season in range(seasons_to_print):
				append_data(
					unit_id,
					season,
					unit.costs[season],
					unit.uses[season])
			append_data(
				unit_id,
				seasons_to_print,
				unit.costs[-1],
				unit.uses[-1])

		fig = px.line(
			data_frame=pd.DataFrame(data),
			x="season",
			y="cost",
			line_group="unit_id",
			color="unit_id",
			hover_data=["value", "type", "uses", "force"],
			title="Unit Cost Adjustment " + adjustment_method 
		)

		pio.write_html(fig, file_name, include_plotlyjs='cdn')


'''
sim = Simulation(adjustment_method_override = "c")
sim.run()
sim.graph("./market_output.html")
# sim.output_equilibrium()
'''


#'''
units = [ Unit(id=id) for id in range(total_units)]
players = [ Player(id=id, units=units) for id in range(total_players)]

units2 = copy.deepcopy(units)
players2 = copy.deepcopy(players)

simA = Simulation(units, players, "c")
simA.run()
simA.graph("./market_output.html")
simA.output_equilibrium()



simB = Simulation(units2, players2, "b")
simB.run()
simB.graph("./market_output_b.html")

#'''

'''
df = px.data.gapminder()

fig = px.line(df, x="year", y="lifeExp", color="continent", line_group="country", hover_name="country",
        line_shape="spline", render_mode="svg")
'''


