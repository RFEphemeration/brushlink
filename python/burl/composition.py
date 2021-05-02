from burl.language import Context, Module, ModuleDictionary, EvaluationError, Value, EvalNode, Parameter
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
	class PathNode:
		def __init__(self, node:EvalNode, index:int):
			self.node = node
			self.index = index

		@property
		def param(self):
			if not self.node or len(self.node.element.parameters) <= self.index:
				return None
			return self.node.element.parameters[self.index]

		@property
		def arg(self):
			if not self.node or not self.node.mapped_arguments or len(self.node.mapped_arguments) <= self.index:
				return None
			if isinstance(self.node.mapped_arguments[self.index], list):
				if not self.node.mapped_arguments[self.index]:
					return None # should we return a previous one?
				else:
					return self.node.mapped_arguments[self.index][-1]

			# should we return a previous one? if the last is none?
			return self.node.mapped_arguments[self.index]

	# this is getting messy, consider re-writing as path and node instead of chain
	def __init__(self, node:EvalNode, path:list = None):
		self.node = node
		self.path = path or []

	def __str__(self,):
		if not self.node:
			return "[_:Any]"
		output = ""
		loc = None
		if self.path:
			loc = self.path[-1].param
		
		visit_stack = [(0, self.node)]
		while visit_stack:
			(ind, visit) = visit_stack.pop()
			if isinstance(visit, EvalNode):
				node = visit
				output += "\n" + ("   " * ind) + node.element.name
				# visiting in reverse order to visit 0th parameter first
				for p in range(len(node.element.parameters) - 1, -1, -1):
					param = node.element.parameters[p]
					has_arg = (p < len(node.mapped_arguments)
						and node.mapped_arguments[p] is not None
						and node.mapped_arguments[p] != [])
					if has_arg and param.repeatable:
						visit_stack.append((ind + 1, param))
						for c in range(len(node.mapped_arguments[p]) - 1, -1, -1):
							visit_stack.append((ind + 1, node.mapped_arguments[p][c]))
					elif has_arg:
						# todo: cursor on element, not just after
						visit_stack.append((ind + 1, node.mapped_arguments[p]))
					else:
						visit_stack.append((ind + 1, param))

			elif isinstance(visit, Parameter):
				param = visit
				# this relies on ref compare which doesn't feel great
				if loc is not None and loc == param:
					output += "\n%s[%s:%s]" % ("   " * ind, loc.name, loc.element_type)
				else:
					output += "\n%s(%s:%s)" % ("   " * ind, param.name, param.element_type)
		# remove first \n
		return output[1:]


	def __repr__(self):
		return self.__str__()

	# internal helpers

	def increment_path_to_open_parameter(self, force_one:bool=False):
		if not self.node:
			self.path = []
			return self
		if not self.path:
			if not self.node.element.parameters:
				self.path = []
				return self
			else:
				self.path.append(Cursor.PathNode(self.node, 0))
		elif force_one:
			self.next_node()

		while self.path:
			tip = self.path[-1]
			is_open = tip.param.repeatable or len(tip.node.mapped_arguments) <= tip.index
			if is_open:
				break
			self.next_node()

		return self

	# exposed functions

	@staticmethod
	def make(node):
		root = Cursor(node)
		root.increment_path_to_open_parameter()
		return root

	def insert_argument(self, node):
		if not self.node or not self.path:
			self.node = node
			self.path = []
		elif self.path:
			tip = self.path[-1]
			tip.node.insert_argument(node, tip.index, sub_index=None)
			# this is either if we inserted an arg to a repeatable param
			# or inserted a more complext tree than a single element
			while tip.arg and tip.arg.element.parameters:
				self.path.append(Cursor.PathNode(tip.arg, 0))
				tip = self.path[-1]

		self.increment_path_to_open_parameter()
		return self

	def get_allowed_argument_types(self):
		if not self.node:
			return frozenset([Value("Any", "Type")])
		elif not self.path:
			return Value(None, "NoneType")

		tip = self.path[-1]
		return frozenset([Value(tip.param.element_type, "Type")])

	def prev_node(self):
		pass

	def next_node(self):
		if not self.path:
			return False
		tip = self.path[-1]
		if len(tip.node.element.parameters) - 1 > tip.index:
			tip.node.skip_param(tip.index)
			tip.index += 1
			return True

		self.path.pop()
		if not self.path:
			return True
		tip = self.path[-1]
		if tip.param.repeatable:
			return True
		else:
			return self.next_node()

	def prev_indent(self):
		if self.path:
			self.path.pop()
			return True
		return False

	def next_indent(self):
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
