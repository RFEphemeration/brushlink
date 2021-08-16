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

class BoxAnchor(Enum):
	center = 0
	upper_left = 1


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
		width=None, height=None, hor=None, vert=None):
		self.top = BoxSize(top[0], BoxUnit[top[1]]) if top else None
		self.bottom = BoxSize(bottom[0], BoxUnit[bottom[1]]) if bottom else None
		self.left = BoxSize(left[0], BoxUnit[left[1]]) if left else None
		self.right = BoxSize(right[0], BoxUnit[right[1]]) if right else None
		self.width = BoxSize(width[0], BoxUnit[width[1]]) if width else None
		self.height = BoxSize(height[0], BoxUnit[height[1]]) if height else None
		self.horizontal = BoxSize(hor[0], BoxUnit[hor[1]]) if hor else None
		self.vertical = BoxSize(vert[0], BoxUnit[vert[1]]) if vert else None
		self.calculated = None

		self.check_validity()

	def check_validity(self):
		def count_dims(dims):
			return reduce(lambda count, dim: count + (1 if dim is not None else 0), dims, 0)

		if count_dims([self.top, self.bottom, self.height, self.vertical]) != 2:
			raise EvaluationError("Box dimensions require exactly two y_axis controls")

		if self.vertical and not self.height:
			raise EvaluationError("Box dimensions that use vertical must use height, not top or bottom")

		if count_dims([self.left, self.right, self.width, self.horizontal]) != 2:
			raise EvaluationError("Box dimensions require exactly two x_axis controls")

		if self.horizontal and not self.width:
			raise EvaluationError("Box dimensions that use horizontal must use width, not left or right")


	# todo: add other relative units like parent size
	def calculate(self, window_calc, parent_calc):
		calc = {}
		for dim in ['top', 'bottom', 'left', 'right', 'width', 'height', 'horizontal', 'vertical']:
			if attribute := getattr(self, dim):
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
		elif self.horizontal:
			x = calc['horizontal'] + parent_calc.x + (parent_calc.width / 2) - width / 2

		x += parent_calc.x

		if self.bottom:
			y = calc['bottom']
		elif self.top:
			y = parent_calc.height - calc['top'] - height
		elif self.vertical:
			y = calc['vertical'] + parent_calc.y + (parent_calc.height / 2) - height / 2

		y += parent_calc.y

		self.calculated = ComputedSize(x, y, width, height)

		return self.calculated


class Panel(pyglet.gui.WidgetBase):
	def __init__(self, wigits,
			name=None,
			box=Box(vert=(0,"px"),hor=(0,"px"),width=(20,"vw"), height=(40,"vh")),
			color=(100,100,100),
			enabled=True):
		super().__init__(0,0,0,0)
		self.enabled = enabled

		self.box = box
		self.background = pyglet.shapes.Rectangle(x=0, y=0, width=0, height=0, color=color)
		self.name = name
		self.wigits = wigits

	def draw(self):
		if not self.enabled:
			return
		self.background.draw()
		for wigit in self.wigits:
			wigit.draw()

	def on_mouse_press(self, x, y, buttons, modifiers):
		if not self.enabled:
			return
		for wigit in self.wigits:
			if getattr(wigit, 'on_mouse_press', None):
				wigit.on_mouse_press(x, y, buttons, modifiers)

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
		size.update_other(self.background, centered=True)
		self.background.anchor_x = self.background.width / 2
		self.background.anchor_y = self.background.height / 2
		for wigit in self.wigits:
			if getattr(wigit, 'update_size', None):
				wigit.update_size(window_calc, size)


class Button(pyglet.gui.WidgetBase):
	def __init__(self, label, box, on_press, font_size = 18, color=(155,155,255)):
		self.box = box
		super().__init__(0,0,0,0);
		self.background = pyglet.shapes.Rectangle(x=0, y=0, width=0, height=0, color=color)
		self.label = pyglet.text.Label(text=label, font_size=font_size, x=0, y=0, anchor_x='center', anchor_y='center');
		self.on_press = on_press
		self.app = None

	def on_mouse_press(self, x, y, buttons, modifiers):
		if self._check_hit(x, y):
			self.on_press(self)

	def set_app(self, app):
		self.app = app

	def draw(self):
		self.background.draw()
		self.label.draw()

	def update_size(self, window_calc, parent_calc):
		old_size = self.box.calculated
		size = self.box.calculate(window_calc, parent_calc)
		#if size == old_size:
		#	return

		size.update_widget(self)
		size.update_other(self.label)
		self.label.y = self.label.y - (self.label.font_size / 2)
		size.update_other(self.background, centered = False)
		#self.background.anchor_x = self.background.width / 2;
		#self.background.anchor_y = self.background.height / 2;


