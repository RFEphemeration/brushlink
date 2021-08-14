from __future__ import print_function, division
from random import seed, randint, uniform


'''
ideas:

separate player rank and deck rank into two axes
'''

player_count = 100
opponent_offset_range = 5

opponent_offset_ratio = 1.2

skill_range = [0,100]

skill_randomness = 5

seed(1)

class Player():
	def __init__(self, index): 
		self.skill = randint(skill_range[0], skill_range[1])
		self.index = index
		self.wins = 0
		self.losses = 0
		self.wl_ratio = 1.0

	def update_wl_ratio(self):
		self.wl_ratio = self.wins / max(float(self.losses + self.wins), 1.0)

	@classmethod
	def battle(cls, one, other):
		one_wins = randomized_battle(one.skill, other.skill)
		if (one_wins):
			one.wins += 1
			other.losses += 1
		else:
			other.wins += 1
			other.losses += 1
		one.update_wl_ratio()
		other.update_wl_ratio()

def fixed_battle(a, b):
	if (a == b):
		return uniform(0.0, 1.0) > 0.5
	return a > b

def randomized_battle(a, b):
	if (a == 0 and b == 0):
		return uniform(0.0, 1.0) > 0.5
	return (uniform(0.0, 1.0) < (a / (a + b)))

players = [Player(i) for i in range(player_count)]

players.sort(reverse=True,key = lambda p : p.wl_ratio)

match_count = 100000

def clamp(n, lower_inc, upper_inc):
	return	max(lower_inc, min(n, upper_inc))

def pick_fixed_opponent(i):
	j = i
	while (j == i):
		j = i + randint(-opponent_offset_range, +opponent_offset_range)
		j = clamp(j, 0, player_count - 1)

def pick_ratio_opponent(i):
	j = i
	lower = max(0, i - 1)
	while (lower > 0
			and players[lower].wl_ratio > (players[i].wl_ratio / opponent_offset_ratio)):
		lower -= 1
	upper = min(player_count - 1, i + 1)
	while (upper < player_count - 1
			and players[upper].wl_ratio < (players[i].wl_ratio * opponent_offset_ratio)):
		upper += 1
	while j == i:
		j = randint(lower, upper)
	return j

for match in range(match_count):
	i = randint(0, player_count - 1)
	j = pick_ratio_opponent(i)

	Player.battle(players[i], players[j])
	players.sort(reverse=True,key = lambda p : p.wl_ratio)

index = 0
for p in players:
	print (p.skill, "%.0f%%" % (p.wl_ratio * 100), sep=':', end=', ')
	index += 1
	if (index == 4):
		print("")
		index = 0

print("")