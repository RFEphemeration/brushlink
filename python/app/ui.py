from collections import namedtuple
from typing import NamedTuple
from functools import reduce
from enum import Enum

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
			x = parent_calc.x + parent_calc.width - calc['right'] - width
		elif self.horizontal:
			x = calc['horizontal'] + parent_calc.x + (parent_calc.width // 2)

		x += parent_calc.x

		if self.bottom:
			y = calc['bottom']
		elif self.top:
			y = parent_calc.y + parent_calc.height - calc['top'] - height
		elif self.vertical:
			y = calc['vertical'] + parent_calc.y + (parent_calc.height // 2)

		y += parent_calc.y

		self.calculated = ComputedSize(x, y, width, height)

		return self.calculated

