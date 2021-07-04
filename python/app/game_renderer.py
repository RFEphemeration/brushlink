from defaultdict import defaultdict
from game import Match, Player, Color

class Point(tuple):
	def __new__(x:int, y:int):
		return tuple.__new__(Point, (x, y))

	@property
	def x(self):
		return self[0]

	@property
	def y(self):
		return self[1]

	def __add__(self, other):
		return Point(self.x + other.x, self.y + other.y)

	def __sub__(self, other):
		return Point(self.x - other.x, self.y - other.y)

	def __div__(self, div):
		return Point(self.x / div, self.y / div)


class Bounds(tuple):
	def __new__(min:Point, max:Point):
		return tuple.__new__(Bounds, (min, max))

	@staticmethod
	def make(x:int, y:int, width:int, height:int):
		return Bounds(Point(x,y), Point(x+width, y+height))

	@property
	def min(self):
		return self[0]

	@property
	def max(self):
		return self[1]

	@property
	def center(self):
		return (self.max + self.min) / 2

	@property
	def size(self):
		return self.max - self.min
	

def bounds_intersect(a, b):
	overlaps = [[],[]]
	for dim in [0, 1]:
		for edge in [0, 1]:
			overlaps[dim].append(a[edge][dim] > b[0][dim] and a[edge][dim] < b[1][dim])

	return (overlaps[0][0] or overlaps[0][1]) and (overlaps[1][0] or overlaps[1][1])

class GameRenderer:
	def __init__(self, match, player_id, grid_scale):
		# definitions
		self.match = match
		self.player_id = player_id
		self.grid_scale = grid_scale

		# load at startup
		self.player_colors = {}
		self.loaded_images = {}
		self.unit_images = defaultdict(dict)
		self.energy_images = defaultdict(dict)

		# runtime_changes
		self.unit_sprites = {}
		self.energy_sprites = {}
		self.unit_batch = pyglet.graphics.Batch()
		self.energy_batch = pyglet.graphics.Batch()

		for player in match.players:
			self.player_colors[player.player_id] = player.color
			for unit_type in match.unit_data:
				unit_settings = match.unit_data[unit_type]
				if unit_settings.images in self.loaded_images:
					image = self.loaded_images[unit_settings.images["body"]]
				else:
					image = pyglet.resources.image(unit_settings.images)
					self.loaded_images[unit_settings.images] = image

				self.unit_images[player.player_id][unit_type] = TextureGrid(ImageGrid(image, len(Color), 1))[player.color]
		for unit_type in match.unit_data:
			# todo: energy images load
			unit_settings = match.unit_data[unit_type]
			image = pyglet.resources.image(unit_esttings.images["energy"])
			self.energy_images[unit_type] = TextureGrid(ImageGrid(image, 1, unit_settings.max_energy))
			pass


	def update_and_draw(self):
		glPushMatrix()
		camera_size = self.match.players[self.player_id].camera_size
		camera_center = self.match.players[self.player_id].camera_size
		camera_bottom_left = (camera_center[0] - (camera_size[0] / 2),
			camera_center[1] - (camera_size[1] / 2))
		camera_top_right = (camera_bottom_left[0] + camera_size[0],
			camera_bottom_left[1] + camera_size[1])
		camera_bounds = (camera_bottom_left, camera_top_right)
		glTranslatef(-camera_bottom_left[0], -camera_bottom_left[1])


		
		# we could iterate through positions
		unit_ids = set()
		

		for unit_id in self.match.units:
			unit = self.match.units[unit_id]

			if not bounds_intersect(camera_bounds, unit.bounds):
				# this unit doesn't get rendered this frame anyways, skip it
				continue
			if not unit_id in self.unit_sprites:
				unit_image = self.unit_images[unit.player_id][unit.settings.unit_type]
				self.unit_sprites[unit.unit_id] = pyglet.sprite.Sprite(unit_image, unit.x, unit.y, self.unit_batch)

				self.energy_spries[unit.unit_id] = pyglet.sprite.Sprite(self.energy_images[unit.energy], unit.x, unit.y, self.energy_batch)
			self.unit_sprites[unit_id].update(x=unit.x, y=unit.y)
			self.energy_sprites[unit_id].image = energy_images[unit.settings.unit_type][unit.energy]

		for unit_id in disjoint(unit_sprites.keys, self.match.units.key):
			del self.unit_sprites[unit_id]
			del self.energy_sprites[unit_id]

		glPopMatrix()

		
		
		