# Elements
class EvaluationError(Exception):
	pass


class Value:
	def __init__(self, value, element_type):
		if isinstance(value, Value):
			raise EvaluationError("value is being nested, was %s, adding type %s" % (value, element_type))
		self.value = value
		self.element_type = element_type

	def __str__(self):
		if self.value is None:
			return "(None): " + self.element_type
		else:
			return str(self.value) + ": " + self.element_type

	def __repr__(self):
		return self.__str__()


class Parameter:
	def __init__(self, name, element_type, default_value=None, required=True, repeatable=False):
		self.name = name
		self.element_type = element_type
		self.default_value = default_value
		self.required = (default_value is None) and required
		self.repeatable = repeatable

	def compile(self, context):
		# todo: compile default code rather than doing so in evaluation
		# and assuming only a single element
		if self.default_value:
			pass

	def accepts(self, element_type):
		# rmf todo: any type matching needs more rigor
		# also polymorphic types
		# also generics
		return self.element_type == element_type or self.element_type == 'Any' or element_type == 'Any'

	@staticmethod
	def make(name, element_type, default_value, repeatable):
		return Parameter(name, element_type, default_value=default_value,repeatable=repeatable)


class Element:
	def __init__(self, name, element_type, parameters = None):
		self.name = name
		self.element_type = element_type
		self.parameters = parameters if parameters is not None else []

	def evaluate(self, context, unevaluated_arguments):
		pass

	def compile(self, context):
		for param in self.parameters:
			param.compile(context)

	def try_append_to_mapped_arguments(self, mapped_arguments, next_argument):
		""" using True, False as Accept, Continue. exceptions are errors """
		param_index = len(mapped_arguments)
		if mapped_arguments and isinstance(mapped_arguments[-1], list):
			param_index -= 1

		for param_index in range(param_index, len(self.parameters)):
			param = self.parameters[param_index]
			# rmf todo: how to deal with unbound names here?
			if param.accepts(next_argument.element.element_type):
				if not param.repeatable:
					mapped_arguments.append(next_argument)
				else:
					if param_index < len(mapped_arguments):
						mapped_arguments[param_index].append(next_argument)
					else:
						mapped_arguments.append([next_argument])
				return True
			if param.required:
				raise EvaluationError("element %s parameter %s has an argument of incorrect type. expects %s, got %s." % (
						self.name, param.name, param.element_type,
						next_argument.element.element_type))
			else:
				if param.repeatable:
					mapped_arguments.append([])
				else:
					# rmf todo: should we evaluate default values yet?
					mapped_arguments.append(None)

		# we may have modified mapped_arguments by filling in default values
		# this should be alright, but will need updating with an undo feature
		return False

	def fill_defaults(self, context, unevaluated_arguments):
		for param_index in range(len(self.parameters)):
			param = self.parameters[param_index]
			if param_index == len(unevaluated_arguments):
				if param.required:
					raise EvaluationError(
							"element %s required parameter %s has no argument" % (
								self.name, param.name))
				if param.repeatable:
					unevaluated_arguments.append([])
				else:
					unevaluated_arguments.append(None)

			if unevaluated_arguments[param_index] == [] and param.repeatable and param.default_value:
				default = context.get_definition(param.default_value)
				unevaluated_arguments[param_index].append(EvalNode(default, []))
			elif unevaluated_arguments[param_index] is None and (not param.repeatable) and param.default_value:
				default = context.get_definition(param.default_value)
				unevaluated_arguments[param_index] = EvalNode(default, [])

		if len(unevaluated_arguments) != len(self.parameters):
			raise EvaluationError("element %s has an incorrect number of parameters" % (self.name))

	def evaluate_arguments(self, context, unevaluated_arguments):
		evaluated_args = []
		for arg in unevaluated_arguments:
			if isinstance(arg, list):
				evaluated_subs = []
				for sub in arg:
					evaluated_subs.append(sub.evaluate(context))
				evaluated_args.append(evaluated_subs)
			else:
				evaluated_args.append(arg.evaluate(context))
		return evaluated_args

	@staticmethod
	def unwrap_values(evaluated_arguments):
		args = []
		for arg in evaluated_arguments:
			if isinstance(arg, list):
				repeatable = []
				for repeat in arg:
					repeatable.append(repeat.value)
				args.append(repeatable)
			else:
				args.append(arg.value)
		return args

class UnboundSymbol(Element):
	def __init__(self, name):
		# consider passing a repeatable not required Any parameter here
		# would probably be a bad idea, this likely means declarations are in order
		Element.__init__(self, name, 'Any')
		self.binding = None

	def evaluate(self, context, unevaluated_arguments):
		if not self.binding:
			symbol_type = context.get_symbol_type(self.name)

			if symbol_type == 'Element':
				self.binding = context.get_definition(self.name)
				self.element_type = self.binding.element_type
			else:
				# ValueName or Type
				self.binding = Literal(self.name, symbol_type, self.name)
				self.element_type = symbol_type

		return self.binding.evaluate(context, unevaluated_arguments)


class Literal(Element):
	def __init__(self, name, element_type, value):
		Element.__init__(self, name, element_type)
		self.value = value

	def evaluate(self, context, unevaluated_arguments):
		if unevaluated_arguments:
			raise EvaluationError("literal %s shouldn't have any arguments." % (self.name))
		if isinstance(self.value, Value):
			return self.value
		else:
			return Value(self.value, self.element_type)


class Handling:
	# argument handling
	PASSTHROUGH = 0
	EVALUATE = 1
	UNWRAP = 2

	# use context
	STANDALONE = 0
	CONTEXT = 1


class Builtin(Element):
	def __init__(self, name, element_type, func, use_context=Handling.CONTEXT, handling=Handling.EVALUATE, parameters=None):
		Element.__init__(self, name, element_type, parameters)
		self.func = func
		self.use_context = use_context
		self.handling = handling

	def evaluate(self, context, unevaluated_arguments):
		if self.handling == Handling.PASSTHROUGH:
			args = unevaluated_arguments
		else:
			args = self.evaluate_arguments(context, unevaluated_arguments)

		if self.handling == Handling.UNWRAP:
			args = Element.unwrap_values(args)

		if self.use_context == Handling.CONTEXT:
			result = self.func(context, *args)
		else:
			result = self.func(*args)

		if isinstance(result, Value):
			return result
		else:
			return Value(result, self.element_type)


class Definition(Element):
	def __init__(self, name, element_type, parameters, code=None, context=None, evaluator=None):
		Element.__init__(self, name, element_type, parameters)
		if evaluator:
			self.evaluator = evaluator
		elif code and context:
			self.evaluator = EvalNode.from_parse_tree(context, ParseNode.parse(code))
		elif code:
			self.code = code
		else:
			# self.evaluator = EvalNode(Literal('None', 'None', None))
			raise Exception("Definitions require either code or evaluator")

	def compile(self, context):
		Element.compile(context)
		if self.code and not self.evaluator:
			self.evaluator = EvalNode.from_parse_tree(ParseNode.parse(context, self.code))

	def evaluate(self, context, unevaluated_arguments):
		args = self.evaluate_arguments(context, unevaluated_arguments)
		values = {}
		for param, arg in zip(self.parameters, args):
			values[param.name] = arg
		child_context = Context(context, values=values)
		return self.evaluator.evaluate(child_context)


# Parsing
class ParseNode:
	def __init__(self, contents, indentation, end_children = None, children = None):
		self.contents = contents
		self.indentation = indentation
		self.end_children = end_children if end_children is not None else []
		self.children = children if children is not None else []

	def __str__(self, indentation=0):
		out = ("   " * indentation) + self.contents
		for child in self.end_children:
			out += "\n" + child.__str__(indentation + 2)
		for child in self.children:
			out += "\n" + child.__str__(indentation + 1)
		return out

	def __repr__(self):
		return self.__str__()

	@staticmethod
	def parse(code):
		nodes = []
		lines = code.splitlines()
		for line in lines:
			contents = line.lstrip('\t')
			indentation = len(line) - len(contents)
			nodes.append(ParseNode(contents, indentation))

		root_node = ParseNode("RootSequence", 0)
		for node in nodes:
			if node.contents == "":
				continue
			current = root_node
			while current.children and node.indentation > (current.indentation + 1):
				current = current.children[-1]
			if node.indentation > current.indentation + 1:
				current.end_children.append(node)
			else:
				current.children.append(node)

		if len(root_node.children) == 1:
			return root_node.children[0]
		else:
			return root_node

	def evaluate(self, context):
		if self.contents == "RootSequence":
			if self.end_children:
				raise EvaluationError("RootSequence has end_children, since it's implicit it should never have things on the same line.")
			# this being hardcoded is probably not ideal? but fine for now
			evaluated_children = []
			for child in self.children:
				child_evaluator = EvalNode.from_parse_tree(context, child)
				evaluated_children.append(child_evaluator.evaluate(context))
			return evaluated_children[-1]
		else:	
			evaluator = EvalNode.from_parse_tree(context, self)
			return evaluator.evaluate(context)

# Evaluation


class EvalNode:
	def __init__(self, element, mapped_arguments = None):
		self.element = element
		self.mapped_arguments = mapped_arguments if mapped_arguments is not None else []

	def __str__(self, indentation = 0):
		output = "   " * indentation + self.element.name
		for arg in self.mapped_arguments:
			if isinstance(arg, list):
				for each in arg:
					output += "\n" + each.__str__(indentation + 1)
			else:
				output += "\n" + arg.__str__(indentation + 1)
		return output

	def __repr__(self):
		return self.__str__()

	def evaluate(self, context):
		# rmf todo: this is going to modify mapped arguments, is that okay?
		self.element.fill_defaults(context, self.mapped_arguments)
		return self.element.evaluate(context, self.mapped_arguments)

	def append_argument(self, arg):
		if self.mapped_arguments:
			if not isinstance(self.mapped_arguments[-1], list):
				success = self.mapped_arguments[-1].append_argument(arg)
				if success:
					return True
			elif self.mapped_arguments[-1]:
				success = self.mapped_arguments[-1][-1].append_argument(arg)
				if success:
					return True

		success = self.element.try_append_to_mapped_arguments(self.mapped_arguments, arg)
		if success:
			return True

		return False

	def get_last_argument(self):
		if not self.mapped_arguments:
			return self
		elif isinstance(self.mapped_arguments[-1], list):
			if self.mapped_arguments[-1]:
				return self.mapped_arguments[-1][-1]
			else:
				# what do we do here?
				raise EvaluationError("mapped_arguments of %s last arg is an empty list" % (self.element.name))
		else:
			return self.mapped_arguments[-1].get_last_argument()

	@staticmethod
	def from_parse_tree(context, parse_node):
		root_node = None
		for name in str.split(parse_node.contents):
			if not name:
				continue
			node = EvalNode.try_bind(context, name)
			if not root_node:
				root_node = node
			else:
				success = root_node.append_argument(node)
				if not success:
					raise EvaluationError("Failed to append argument")

		last_node = root_node.get_last_argument()
		if last_node == root_node and parse_node.end_children:
			raise EvaluationError("Element %s has end children but only one element on the line" % root_node.element.name)

		for child in parse_node.end_children:
			child_node = EvalNode.from_parse_tree(context, child)
			success = last_node.element.try_append_to_mapped_arguments(last_node.mapped_arguments, child_node)
			if not success:
				raise EvaluationError("incorrect argument")

		for child in parse_node.children:
			child_node = EvalNode.from_parse_tree(context, child)
			success = root_node.element.try_append_to_mapped_arguments(root_node.mapped_arguments, child_node)
			if not success:
				raise EvaluationError("incorrect argument")
		return root_node

	@staticmethod
	def try_bind(context, name):
		if name.isdigit():
			element = Literal(name, 'Number', int(name))
		elif name[0] == "-":
			element = Literal(name, 'Number', -1 * int(name[1:]))
		else:
			try:
				element = context.get_definition(name)
			except EvaluationError:
				if context.is_known_type(name):
					element = Literal(name, 'Type', name)
				else:
					element = UnboundSymbol(name)
		return EvalNode(element)


# Context and builtins
class Context:
	def __init__(self, parent, values = None, definitions = None, types = None):
		self.parent = parent
		self.values = values if values is not None else {}
		self.definitions = definitions if definitions is not None else {}
		self.types = types if types is not None else {}

	# internal helpers

	def get_definition(self, name):
		context = self
		while context:
			if name in context.definitions:
				return context.definitions[name]
			else:
				context = context.parent
		raise EvaluationError("get failed, no definition found with name %s" % (name))

	def is_known_type(self, name):
		context = self
		while context:
			if name in context.types:
				return True
			else:
				context = context.parent
		return False

	def get_symbol_type(self, name):
		context = self
		while context:
			if name in context.types:
				return 'Type'
			elif name in context.definitions:
				return 'Element'
			elif name in context.values:
				return 'ValueName'
			else:
				context = context.parent
		# assume unrecognized symbols are new value names, does this hold?
		return 'ValueName'

	# builtin functions

	def define(self, name, element_type, parameters, evaluator):
		eval_name = name.evaluate(self).value
		eval_type = element_type.evaluate(self).value
		eval_params = []
		for param in parameters:
			eval_params.append(param.evaluate(self).value)
		root_node = EvalNode(self.get_definition('Sequence'), [evaluator])
		definition = Definition(eval_name, eval_type, eval_params, evaluator=root_node)
		self.definitions[eval_name] = definition
		# rmf todo: should this return the definition? or just nothing
		return definition

	def set_local(self, name, value):
		self.values[name.value] = value
		return value

	def set_global(self, name, value):
		context = self
		while context.parent:
			context = context.parent
		context.values[name.value] = value
		return value

	def get(self, name):
		context = self
		while context:
			if name.value in context.values:
				return context.values[name.value]
			else:
				context = context.parent
		raise EvaluationError("get failed, no value found with name %s" % (name.value))

	def for_loop(self, count, index_name, expression):
		value = Value(None, 'None')
		eval_index_name = index_name.evaluate(self).value
		for i in range(count.evaluate(self).value):
			if eval_index_name:
				set_local(self, eval_index_name, Value(i, 'Number'))
			value = expression.evaluate(self)
		return value

	def for_each_loop(self, values, index_name, expression):
		eval_index_name = index_name.evaluate(self).value
		eval_values = []
		for value in values:
			eval_values.append(value.evaluate(self))

		ret = Value(None, 'None')
		for value in eval_value:
			set_local(self, eval_index_name, value)
			ret = value.evaluate(self)
		return ret

	def while_loop(self, condition, expression):
		value = Value(None, 'None')
		while condition.evaluate(self).value:
			value = expression.evaluate(self)
		return value

	def if_branch(self, condition, consequent, alternative):
		if condition.evaluate(self).value:
			return consequent.evaluate(self).value
		else:
			return alternative.evaluate(self).value

	def logical_not(self, value):
		return Value(not value.value, 'Boolean')

	def logical_all(self, expressions):
		for expr in expressions:
			if not expr.evaluate(self).value:
				return Value(False, 'Boolean')
		return Value(True, 'Boolean')

	def logical_any(self, expressions):
		for expr in expressions:
			if expr.evaluate(self).value:
				return Value(True, 'Boolean')
		return Value(False, 'Boolean')



def element_sum(operand, operands):
	return sum(operands, operand)


def sequence(expressions):	
	return expressions[-1]

class Comparison:
	EQ = 0
	NE = 1
	LT = 2
	GT = 3
	LT_EQ = 4
	GT_EQ = 5


def compare(left, comparison, right):
	comparators = {
		Comparison.EQ: lambda x, y: x == y,
		Comparison.NE: lambda x, y: x != y,
		Comparison.LT: lambda x, y: x < y,
		Comparison.GT: lambda x, y: x > y,
		Comparison.LT_EQ: lambda x, y: x <= y,
		Comparison.GT_EQ: lambda x, y: x >= y
	}
	return comparators[comparison](left, right)

root = Context(None, types={
		'Any',
		'None',
		'Type',
		'ValueName',
		'Boolean',
		'Number',
		'Comparison',
		'Parameter',
		'Element',
	},
	definitions={
		'RootSequence': Builtin('RootSequence', 'Any', sequence, Handling.STANDALONE, parameters=[
			Parameter('expressions', 'Any', repeatable=True)
		]),
		'Sequence': Builtin('Sequence', 'Any', sequence, Handling.STANDALONE, parameters=[
			Parameter('expressions', 'Any', repeatable=True)
		]),
		'For': Builtin('For', 'Any', Context.for_loop, handling=Handling.PASSTHROUGH, parameters=[
			Parameter('count', 'Number'),
			Parameter('value_name', 'ValueName', default_value='None'),
			Parameter('expression', 'Any'),
		]),
		'ForEach': Builtin('ForEach', 'Any', Context.for_each_loop, handling=Handling.PASSTHROUGH, parameters=[
			Parameter('values', 'Any', repeatable=True),
			Parameter('value_name', 'ValueName'),
			Parameter('expression', 'Any'),
		]),
		'While': Builtin('While', 'Any', Context.while_loop, handling=Handling.PASSTHROUGH, parameters=[
			Parameter('condition', 'Boolean'),
			Parameter('expression', 'Any'),
		]),
		'If': Builtin('If', 'Any', Context.if_branch, handling=Handling.PASSTHROUGH, parameters=[
			Parameter('condition', 'Boolean'),
			Parameter('consequent', 'Any'),
			Parameter('alternative', 'Any', default_value='None'),
		]),
		'SetLocal': Builtin('SetLocal', 'None', Context.set_local, parameters=[
			Parameter('name', 'ValueName'),
			Parameter('value', 'Any'),
		]),
		'SetGlobal': Builtin('SetGlobal', 'None', Context.set_global, parameters=[
			Parameter('name', 'ValueName'),
			Parameter('value', 'Any'),
		]),
		'Get': Builtin('Get', 'Any', Context.get, parameters=[
			Parameter('name', 'ValueName'),
		]),
		'True': Literal('True', 'Boolean', True),
		'False': Literal('False', 'Boolean', False),
		'Not': Builtin('Not', 'Boolean', Context.logical_not, parameters=[
			Parameter('value', 'Boolean'),
		]),
		'All': Builtin('All', 'Boolean', Context.logical_all, handling=Handling.PASSTHROUGH, parameters=[
			Parameter('expressions', 'Boolean', repeatable='True'),
		]), 
		'Any': Builtin('Any', 'Boolean', Context.logical_any, handling=Handling.PASSTHROUGH, parameters=[
			Parameter('expressions', 'Boolean', repeatable='True'),
		]),
		'None': Literal('None', 'none', None),
		'Parameter': Builtin('Parameter', 'Parameter', Parameter.make, Handling.STANDALONE, Handling.UNWRAP, [
			# rmf todo: default name based on index
			Parameter('name', 'ValueName'),
			# rmf todo: type disambiguation from definitions
			Parameter('element_type', 'Type'),
			# rmf todo: generics/polymorphism, this should match element_type
			Parameter('default_value', 'Any', default_value='None'),
			Parameter('repeatable', 'Boolean', default_value='False'),
		]),
		'Define': Builtin('Define', 'Element', Context.define, handling=Handling.PASSTHROUGH, parameters=[
			Parameter('name', 'ValueName'),
			Parameter('element_type', 'Type'),
			Parameter('parameters', 'Parameter', required=False, repeatable=True),
			# rmf todo: generics/polymorphism, this should match element_type
			Parameter('code', 'Any', repeatable=True),
		]),
		'Sum': Builtin('Sum', 'Number', sum, Handling.STANDALONE, Handling.UNWRAP, [
			Parameter('operands', 'Number', repeatable=True)
		]),
		'==': Literal('==', 'Comparison', Comparison.EQ),
		'/=': Literal('/=', 'Comparison', Comparison.NE),
		'<': Literal('<', 'Comparison', Comparison.LT),
		'>': Literal('<', 'Comparison', Comparison.GT),
		'<=': Literal('<=', 'Comparison', Comparison.LT_EQ),
		'>=': Literal('>=', 'Comparison', Comparison.GT_EQ),
		'Compare': Builtin('Compare', 'Boolean', compare, Handling.STANDALONE, Handling.UNWRAP, [
			Parameter('left', 'Number'),
			Parameter('comparison', 'Comparison', default_value='=='),
			Parameter('right', 'Number'),
			]),
	})


# todo:
"""
you can't use anything as a value name after it has been defined as a definition or type
is that okay?


"""

