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
	values = {}

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
			return Value(tree.value, 'EvalNode')
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
			return None

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
		if self.node.element.parameters[param_index].repeatable:
			# should we append an empty list already?
			# or keep sub_index at None?
			sub_index = 0

		self.child = Cursor(None, parent=self, param_index=len(args), sub_index=sub_index)

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
			return
		self.node = node
		if self.parent:
			self.parent.node.insert_argument(self.node, self.param_index, self.sub_index)

		self.get_root().extend_child_to_open_parameter()


	def prev_node(self):
		pass

	def next_node(self):
		pass

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
		[append_argument, """Builtin AppendArgument EvalNode Standalone
	Parameter tree EvalNode
	Parameter argument EvalNode"""],
		[Cursor.make, """Builtin Cursor.Make Cursor Standalone Unwrap Parameter tree EvalNode"""],
		[Cursor.insert_argument, """Builtin InsertArgument Cursor Standalone Unwrap
	Parameter cursor Cursor
	Parameter arg EvalNode"""],
	],
)))
