from burl.language import Context, Module, ModuleDictionary, EvaluationError, Value, EvalNode
from burl.builtin import builtin_tools, make_context


def all_types(current_context):
	types = set()

	def merge_types(context):
		nonlocal types
		for t in context.types:
			types.add(Value(t, 'Type'))
		return False # navigate to full depth
	current_context.navigate_parents_and_modules(merge_types)
	return frozenset(types)


def definitions_of_type(current_context, element_type):
	definitions = set()

	def merge_definitions(context):
		nonlocal definitions
		# what to do about name conflicts?
		for name in context.definitions:
			# do we want elements that a parameter of type would accept?
			if context.definitions[name].element_type == element_type.value:
				# what is the type here? symbol name?
				definitions.add(Value(name, 'ValueName'))
		return False # navigate to full depth
	current_context.navigate_parents_and_modules(merge_definitions)
	return frozenset(definitions)


def values_of_type(current_context, element_type):
	values = set()

	def merge_values(context):
		nonlocal values
		# what to do about name conflicts?
		for name in context.values:
			# do we want elements that a parameter of type would accept?
			if context.values[name].element_type == element_type.value:
				values.add(Value(name, 'ValueName'))
		return False # navigate to full depth
	current_context.navigate_parents_and_modules(merge_values)
	return frozenset(values)


def evaluate(context, node):
	if node.element_type == 'EvaluationError':
		return node
	try:
		value = node.value.evaluate(context)
		if isinstance(value, Value):
			return value
		else:
			return Value(value, node.value.element.element_type)
	except EvaluationError as e:
		return Value(e, 'EvaluationError')


def append_argument(tree, arg):
	# this modifies the argument, which is not ideal
	try:
		(success, skips_remaining) = tree.value.append_argument(arg.value, 0)
		if success:
			return tree
		else:
			return Value('Could not append argument %s to %s' % (
				arg.value.element.name, tree.value.element.name), 'EvaluationError')
	except EvaluationError as e:
		return Value(e, 'EvaluationError')


class Cursor:
	def __init__(self, node:EvalNode, parent=None, param_index:int=None, sub_index:int=None):
		self.node = node
		self.parent = parent
		self.param_index = param_index
		self.sub_index = sub_index
		self.child = None

	def __str__(self, indentation = 0):
		prefix = "   " * indentation
		if not self.child:
			if self.node:
				return prefix + "[" + self.node.element.name + "]"
			else:
				if self.parent and self.param_index is not None:
					param = self.parent.node.element.parameters[self.param_index]
					return prefix + "[" + param.name + ": " + param.element_type + "]"
				else:
					return prefix + "[_]"
		
		output = prefix + self.node.element.name
		prefix += "   "

		if not self.node:
			raise EvaluationError("Cursor with child has no node...")

		for p in range(len(self.node.element.parameters)):
			param = self.node.element.parameters[p]
			if self.child and p == self.child.param_index:
				if self.child.sub_index:
					for s in range(len(self.node.mapped_arguments[p])):
						if s == self.child.sub_index:
							output += "\n" + self.child.__str__(indentation + 1)
						else:
							output += "\n" + self.node.mapped_arguments[p][s].__str__(indentation + 1)
				else:
					output += "\n" + self.child.__str__(indentation + 1)
			elif p >= len(self.node.mapped_arguments):
				output += "\n" + prefix + "(" + param.name + ": " + param.element_type + ")"
			else:
				if isinstance(self.node.mapped_arguments[p], list):
					for sub_arg in self.node.mapped_arguments[p]:
						output += "\n" + sub_arg.__str__(indentation + 1)
				else:
					output += "\n" + self.node.mapped_arguments[p].__str__(indentation + 1)

		return output

	def __repr__(self):
		return self.__str__()

	# internal helpers

	def extend_child_to_open_parameter(self):
		if self.child:
			self.child = self.child.extend_child_to_open_parameter()
			if self.child:
				# our existing child still has an open parameter
				return self
		if not self.node:
			return self # this is an open parameter
		if not self.node.element.parameters:
			return None # this has no parameters

		args = self.node.mapped_arguments
		child_node = None
		index = 0
		sub_index = None
		if args:
			index = len(args)-1
			child_node = args[-1]
			if isinstance(args[-1], list):
				if not args[-1]:
					child_node = None
					sub_index = 0
				else:
					child_node = args[-1][-1]
					sub_index = len(args[-1])-1

		child = Cursor(child_node, parent=self, param_index=index, sub_index=sub_index)
		self.child = child.extend_child_to_open_parameter()
		if self.child:
			# the last argument has an open parameter child
			return self

		# the last argument has no open parameters

		if len(self.node.mapped_arguments) >= len(self.node.element.parameters):
			# there are no more available parameters
			return None

		sub_index = None
		if self.node.element.parameters[self.param_index].repeatable:
			# should we append an empty list already?
			# or keep sub_index at None?
			self.sub_index = 0

		self.child = Cursor(None, parent=self, param_index=len(args), sub_index=sub_index)
		return self

	def get_root(self):
		cursor = self
		while cursor.parent:
			cursor = cursor.parent
		return cursor

	def update_node(self, param_index, sub_index):
		self.child = None

	# exposed functions

	@staticmethod
	def make(node):
		root = Cursor(node)
		root.extend_child_to_open_parameter()
		return root

	def insert_argument(self, node):
		if self.child:
			self.child.insert_argument(node)
			return self
		self.node = node
		if self.parent:
			self.parent.node.insert_argument(self.node, self.param_index, self.sub_index)

		self.get_root().extend_child_to_open_parameter()
		return self

	def get_allowed_argument_types(self):
		if self.child:
			return self.child.get_allowed_argument_types()

		if self.node or not self.parent:
			return Value(None, "NoneType")

		# should we do anything about Any?
		return frozenset([Value(self.parent.node.element.parameters[self.param_index].element_type, "Type")])

	def prev_node(self):
		pass

	def next_node(self):
		if self.child:
			if self.child.next_node():
				return True
			else:
				raise EvaluationError("Cursor next child is unimplemented")

		if not self.parent:
			return False

		try:
			if not self.node:
				self.parent.node.skip_param(self.param_index)
			self.param_index += 1
			self.sub_index = None
			self.node = None
			return True
		except EvaluationError as e:
			return False


	def move_up(self):
		pass

	def move_down(self):
		pass

	def delete_branch(self):
		pass


ModuleDictionary.instance().add_module(Module('composition', context=make_context(
	temp_parent=builtin_tools,
	types={
		'EvaluationError',
		'Cursor',
	},
	evaluations=[
		[None, "LoadModule collections"],
		[all_types, "Builtin AllTypes HashSet Contextual"],
		[definitions_of_type, "Builtin DefinitionsOfType HashSet Contextual Parameter type Type"],
		[values_of_type, "Builtin ValuesOfType HashSet Contextual Parameter type Type"],
		[evaluate, "Builtin Evaluate Any Contextual Parameter node EvalNode"],
		[append_argument, """Builtin AppendArgument EvalNode
	Parameter tree EvalNode
	Parameter argument EvalNode"""],
		[Cursor.make, """Builtin Cursor.Make Cursor Unwrap Parameter tree EvalNode"""],
		[lambda c: c.node, """Builtin Cursor.GetEvalNode EvalNode Unwrap Parameter cursor Cursor"""],
		[Cursor.insert_argument, """Builtin Cursor.InsertArgument Cursor Unwrap
	Parameter cursor Cursor
	Parameter arg EvalNode"""],
		[Cursor.get_allowed_argument_types, """Builtin Cursor.GetAllowedArgumentTypes HashSet Unwrap
	Parameter cursor Cursor"""],
		[None, """Define Cursor.NextAllowingType Cursor
	Parameter cursor Cursor
	Parameter type Type
	Skip
	While Not HashSet.Contains
			Cursor.GetAllowedArgumentTypes Get cursor
			Get type
		Cursor.InsertArgument Get cursor Quote Skip # should use move_next instead of insert skip
	Get cursor"""],
	],
)))
