from collections import namedtuple
from typing import NamedTuple
from functools import reduce
from enum import Enum
import pyglet

class BoxUnit(Enum):
	px = 0 # pixels
	vmin = 1 # window min / 60
	vw = 3 # window width / 60
	vh = 4 # window height / 60
	pw = 5 # parent width / 60
	ph = 6 # parent height / 60

class BoxAnchor(Enum):
	center = 0
	upper_left = 1

class Axis(Enum):
	vertical = 0
	horizontal = 1


class BoxSize():
	def __init__(self, value, unit):
		self.value = value
		self.unit = unit

	def calculate(self, window_calc, parent_calc):
		if self.unit == BoxUnit.px:
			return self.value
		elif self.unit == BoxUnit.vw:
			return self.value * window_calc.width / 60.0 
		elif self.unit == BoxUnit.vh:
			return self.value * window_calc.height / 60.0
		elif self.unit == BoxUnit.vmin:
			return self.value * min(window_calc.height, window_calc.width) / 60.0
		elif self.unit == BoxUnit.pw:
			return self.value * parent_calc.width / 60.0
		elif self.unit == BoxUnit.ph:
			return self.value * parent_calc.height / 60.0
		else:
			return self.value


class ComputedSize(NamedTuple):
	x : float = 0
	y : float = 0
	width : float = 0
	height : float = 0

	def update_other(self, other, centered=True):
		if hasattr(other, 'width'):
			other.width = self.width
		if hasattr(other, 'height'):
			other.height = self.height
		if centered:
			other.x = self.x + (self.width / 2)
			other.y = self.y + (self.height / 2)
		else:
			other.x = self.x
			other.y = self.y

	def update_widget(self, other):
		other._width = self.width
		other._height = self.height
		other._x = self.x
		other._y = self.y
			

class Box():
	def __init__(self, top=None, bottom=None, left=None, right=None,
		width=None, height=None, over=None, up=None):
		self.top = BoxSize(top[0], BoxUnit[top[1]]) if top else None
		self.bottom = BoxSize(bottom[0], BoxUnit[bottom[1]]) if bottom else None
		self.left = BoxSize(left[0], BoxUnit[left[1]]) if left else None
		self.right = BoxSize(right[0], BoxUnit[right[1]]) if right else None
		self.width = BoxSize(width[0], BoxUnit[width[1]]) if width else None
		self.height = BoxSize(height[0], BoxUnit[height[1]]) if height else None
		self.over = BoxSize(over[0], BoxUnit[over[1]]) if over else None
		self.up = BoxSize(up[0], BoxUnit[up[1]]) if up else None
		self.calculated = None

		self.check_validity()

	@staticmethod
	def Fill(margin=0):
		return Box(top=(margin,'px'),bottom=(margin,'px'),left=(margin,'px'),right=(margin,'px'))

	def check_validity(self):
		def count_dims(dims):
			return reduce(lambda count, dim: count + (1 if dim is not None else 0), dims, 0)

		if count_dims([self.top, self.bottom, self.height, self.up]) != 2:
			raise EvaluationError("Box dimensions require exactly two y_axis controls")

		if self.up and not self.height:
			raise EvaluationError("Box dimensions that use up must use height, not top or bottom")

		if count_dims([self.left, self.right, self.width, self.over]) != 2:
			raise EvaluationError("Box dimensions require exactly two x_axis controls")

		if self.over and not self.width:
			raise EvaluationError("Box dimensions that use over must use width, not left or right")


	# todo: add other relative units like parent size
	def calculate(self, window_calc, parent_calc):
		calc = {}
		for dim in ['top', 'bottom', 'left', 'right', 'width', 'height', 'over', 'up']:
			if attribute := getattr(self, dim, None):
				calc[dim] = attribute.calculate(window_calc, parent_calc)

		width = None
		height = None
		x = None
		y = None

		if self.width:
			width = calc['width']
		else:
			width = parent_calc.width - calc['right'] - calc['left']

		if self.height:
			height = calc['height']
		else:
			height = parent_calc.height - calc['top'] - calc['bottom']

		if self.left:
			x = calc['left']
		elif self.right:
			x = parent_calc.width - calc['right'] - width
		elif self.over:
			x = calc['over'] + parent_calc.x + (parent_calc.width / 2) - width / 2

		x += parent_calc.x

		if self.bottom:
			y = calc['bottom']
		elif self.top:
			y = parent_calc.height - calc['top'] - height
		elif self.up:
			y = calc['up'] + parent_calc.y + (parent_calc.height / 2) - height / 2

		y += parent_calc.y

		self.calculated = ComputedSize(x, y, width, height)

		return self.calculated


class Panel(pyglet.gui.WidgetBase):
	def __init__(self, wigits,
			name=None,
			box=Box(up=(0,"px"),over=(0,"px"),width=(20,"vw"), height=(40,"vh")),
			color=None,
			enabled=True,
			block_input=None):
		super().__init__(0,0,0,0)
		self.enabled = enabled

		self.box = box
		self.background = None if color is None else pyglet.shapes.Rectangle(x=0, y=0, width=0, height=0, color=color)
		self.name = name
		self.wigits = wigits
		self.block_input = block_input if block_input is not None else (color is not None)

	def draw(self):
		if not self.enabled:
			return
		if self.background is not None:
			self.background.draw()
		for wigit in self.wigits:
			wigit.draw()

	def on_mouse_press(self, x, y, buttons, modifiers):
		if not self.enabled:
			return False
		for wigit in reversed(self.wigits):
			if hasattr(wigit, 'on_mouse_press'):
				consumed = wigit.on_mouse_press(x, y, buttons, modifiers)
				if consumed:
					return True
		if self.block_input and self._check_hit(x, y):
			return True
		return False

	def show(self):
		self.enabled = True

	def hide(self):
		self.enabled = False

	def update_size(self, window_calc, parent_calc):
		old_size = self.box.calculated
		size = self.box.calculate(window_calc, parent_calc)
		if size == old_size:
			return
		size.update_widget(self)
		if self.background is not None:
			size.update_other(self.background, centered=True)
			self.background.anchor_x = self.background.width / 2
			self.background.anchor_y = self.background.height / 2
		for wigit in self.wigits:
			if hasattr(wigit, 'update_size'):
				wigit.update_size(window_calc, size)


class Button(pyglet.gui.WidgetBase):
	def __init__(self, label, box, on_press, font_size=(18,"px"), color=(155,155,255)):
		self.box = box
		super().__init__(0,0,0,0);
		self.font_size = BoxSize(font_size[0], BoxUnit[font_size[1]])
		self.background = pyglet.shapes.Rectangle(x=0, y=0, width=0, height=0, color=color)
		self.label = pyglet.text.Label(text=label, font_size=12, x=0, y=0, anchor_x='center', anchor_y='center')
		self.on_press = on_press

	def on_mouse_press(self, x, y, buttons, modifiers):
		if self._check_hit(x, y):
			self.on_press(self)
			return True
		return False

	def draw(self):
		self.background.draw()
		self.label.draw()

	def update_size(self, window_calc, parent_calc):
		old_size = self.box.calculated
		size = self.box.calculate(window_calc, parent_calc)
		if size == old_size:
			return

		size.update_widget(self)
		size.update_other(self.label)
		self.label.font_size = self.font_size.calculate(window_calc, parent_calc)
		self.label.height = self.label.font_size
		size.update_other(self.background, centered = False)


class Label:
	def __init__(self, label, box, font_size=(18,'px')):
		self.box = box
		self.font_size = BoxSize(font_size[0], BoxUnit[font_size[1]])
		self.label = pyglet.text.Label(text=label, font_size=12, x=0, y=0, anchor_x='center', anchor_y='center')

	def update_size(self, window_calc, parent_calc):
		old_size = self.box.calculated
		size = self.box.calculate(window_calc, parent_calc)
		if size == old_size:
			return

		size.update_other(self.label)
		self.label.font_size = self.font_size.calculate(window_calc, parent_calc)
		self.label.height = self.label.font_size

	def draw(self):
		self.label.draw()


class Tabs:
	def __init__(self, box, contents):
		self.box = box
		self.contents = contents
		for content in self.contents:
			if hasattr(content, 'enabled'):
				content.enabled = False
		self.active_tab = 0

	def make_tabs(self):
		#rmf todo: do we need to delete existing tabs in some way?
		self.tabs = []
		for name in self.tab_names:
			self.tabs.append(Button())

	def set_active_tab(self, active_tab):
		self.contents[self.active_tab].enabled = False
		self.active_tab = active_tab
		self.contents[active_tab].enabled = True

	def draw(self):
		self.contents[self.active_tab].draw()

	def update_size(self, window_calc, parent_calc):
		old_size = self.box.calculated
		size = self.box.calculate(window_calc, parent_calc)
		if size == old_size:
			return

		for content in self.contents:
			if hasattr(content, 'update_size'):
				content.update_size(window_calc, size)

	def on_mouse_press(self, x, y, buttons, modifiers):
		if hasattr(self.contents[self.active_tab], 'on_mouse_press'):
			return self.contents[self.active_tab].on_mouse_press(x, y, buttons, modifiers)
		return False

