from defaultdict import defaultdict


class GameRenderer:
	def __init__(self, match, player_id):
		self.match = match
		self.player_id = player_id
		self.unit_sprites = {}
		self.unit_energy_sprites = {}
		self.player_colors = {}
		self.loaded_images = {}
		self.unit_images = defaultdict(dict)

		for player in match.players:
			self.player_colors[player.player_id] = player.color
			for unit_type in match.units:
				unit_settings = match.units[unit_type]
				if unit_settings.images in self.loaded_images:
					image = self.loaded_images[unit_settings.images]
				else:
					image = pyglet.resources.image(unit_settings.images)
					self.loaded_images[unit_settings.images] = image

				self.unit_images[player.player_id][unit_type] = TextureGrid(ImageGrid(image, 1, len(Player.COLORS)))[player.color]


	def update(self):
		for unit_id in self.match.units:
			unit = self.match.units[unit_id]
			if not unit_id in self.unit_sprites:
				unit_image = self.unit_images[unit.player_id][unit.settings.unit_type]
				self.unit_sprites[unit.unit_id] = unit_image
			self.unit_sprites[unit_id].update(x=unit.position[0], y=unit.position[1])

		for unit_id in self.match.deleted_units:
			del self.unit_sprites[unit_id]

	def draw(self):
		glPushMatrix()
		camera_bottom_left = self.match.players[self.player_id].camera_bottom_left
		glTranslatef(-camera_bottom_left[0], -camera_bottom_left[1])

		# units
		for sprite in self.unit_sprites:
			sprite.draw()


		glPopMatrix()
		
		