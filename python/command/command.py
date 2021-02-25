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

	def match_args_to_params(self, context, unevaluated_arguments):
		args = []
		arg_index = 0
		for param_index in range(len(self.parameters)):
			param = self.parameters[param_index]
			while arg_index < len(unevaluated_arguments):
				arg = unevaluated_arguments[arg_index]
				if not param.accepts(arg.element.element_type):
					break

				if not param.repeatable:
					args.append(arg)
					arg_index += 1
					break

				elif param.repeatable:
					if param_index == len(args) - 1:
						# we have already started this parameter list
						args[-1].append(arg)
					else:
						args.append([arg])
					arg_index += 1

			# we haven't yet added an argument for this parameter
			if param_index >= len(args):
				if param.default_value:
					default = context.get_definition(param.default_value)
					args.append(EvalNode(default, []))
				elif not param.required:
					if param.repeatable:
						args.append([])
					else:
						args.append(EvalNode(Literal('None', 'None', None), []))
				else:
					if arg_index < len(unevaluated_arguments):
						raise EvaluationError("element %s parameter %s has an argument of incorrect type. expects %s, got %s." % (
								self.name, param.name, param.element_type,
								unevaluated_arguments[arg_index].element.element_type))
					else:
						raise EvaluationError(
							"element %s required parameter %s has no argument" % (
								self.name, param.name))
			
		if len(args) != len(self.parameters):
			raise EvaluationError("element %s has an incorrect number of parameters" % (self.name))
		return args

	def evaluate_arguments(self, context, unevaluated_arguments):
		unevaluated_args = self.match_args_to_params(context, unevaluated_arguments)
		evaluated_args = []
		for arg in unevaluated_args:
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
		# passing a repeatable not required Any parameter here
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
			args = self.match_args_to_params(context, unevaluated_arguments)
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
			self.evaluator = EvalNode.make(context, ParseNode.parse(code))
		elif code:
			self.code = code
		else:
			raise Exception("Definitions require either code or evaluator")
			#self.evaluator = EvalNode(Literal('None', 'None', None))

	def compile(self, context):
		Element.compile(context)
		if self.code and not self.evaluator:
			self.evaluator = EvalNode.make(ParseNode.parse(context, self.code))

	def evaluate(self, context, unevaluated_arguments):
		args = self.evaluate_arguments(context, unevaluated_arguments)
		values = {}
		for param, arg in zip(self.parameters, args):
			values[param.name] = arg
		child_context = Context(context, values=values)
		return self.evaluator.evaluate(child_context)


# Parsing
class ParseNode:
	def __init__(self, contents, indentation, parent = None, children = None):
		self.contents = contents
		self.indentation = indentation
		self.parent = parent
		self.children = children if children is not None else []

	def __str__(self, indentation = 0):
		out = ("   " * indentation) + self.contents
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
			if nodes and indentation >= nodes[-1].indentation + 2:
				# this is a continuation of the previous line, not a new node
				if nodes[-1].contents:
					nodes[-1].contents += " "
				nodes[-1].contents += contents
			else:
				nodes.append(ParseNode(contents, indentation))

		root = ParseNode("Sequence", 0)
		for node in nodes:
			if node.contents == "":
				continue
			current = root
			while current.children and node.indentation > (current.indentation + 1):
				current = current.children[-1]
			current.children.append(node)
			node.parent = current

		if len(root.children) == 1:
			return root.children[0]
		else:
			return root

	def evaluate(self, context):
		evaluator = EvalNode.make(context, self)
		return evaluator.evaluate(context)

# Evaluation


class EvalNode:
	def __init__(self, element, arguments = None):
		self.element = element
		self.arguments = arguments if arguments is not None else []

	def evaluate(self, context):
		return self.element.evaluate(context, self.arguments)
		#if isinstance(self.element, Element):	

	@staticmethod
	def make(context, parse_node):
		try:
			element = context.get_definition(parse_node.contents)
		except:
			if context.is_known_type(parse_node.contents):
				element = Literal(parse_node.contents, 'Type', parse_node.contents)
			else:
				element = UnboundSymbol(parse_node.contents)
		node = EvalNode(element)
		# rmf todo: map arguments to params, if param is of type name, are all names literals?
		for child in parse_node.children:
			node.arguments.append(EvalNode.make(context, child))
		return node

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
		sequence = EvalNode(self.get_definition('Sequence'), evaluator)
		definition = Definition(eval_name, eval_type, eval_params, evaluator=sequence)
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


def element_sum(operand, operands):
	return sum(operands, operand)


def sequence(expressions):	
	return expressions[-1]


root = Context(None, types={
		'Any',
		'None',
		'Type',
		'ValueName',
		'Boolean',
		'Number',
		'Parameter',
		'Element',
	},
	definitions={
		'Sequence': Builtin('Sequence', 'Any', sequence, Handling.STANDALONE, parameters=[
			Parameter('expressions', 'Any', repeatable=True)
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
		'None': Literal('None', 'none', None),
		'Parameter': Builtin('Parameter', 'Parameter', Parameter.make, Handling.STANDALONE, Handling.UNWRAP, [
			# rmf todo: default name based on index
			Parameter('name', 'ValueName'),
			# rmf todo: type disambiguation from definitions
			Parameter('element_type', 'Type'),
			# rmf todo: generics/polymorphism, this should match element_type
			Parameter('default_value', 'Any', required=False),
			Parameter('repeatable', 'Boolean', default_value='False'),
		]),
		'Define': Builtin('Define', 'Element', Context.define, handling=Handling.PASSTHROUGH, parameters=[
			Parameter('name', 'ValueName'),
			Parameter('element_type', 'Type'),
			Parameter('parameters', 'Parameter', required=False, repeatable=True),
			# rmf todo: generics/polymorphism, this should match element_type
			Parameter('code', 'Any', repeatable=True),
		]),
		'one': Literal('one', 'Number', 1),
		'Sum': Builtin('Sum', 'Number', sum, Handling.STANDALONE, Handling.UNWRAP, [
			Parameter('operands', 'Number', repeatable=True)
		]),
	})

root.definitions['AddOne'] = Definition('AddOne', 'Number', [
		Parameter('value', 'Number')
	],
	context=root,
	code="""
	Sum
		Get
			value
		one
	""")

# Testing


def main():
	print root.definitions.keys()
	ast = ParseNode.parse("""
	SetLocal
		hi
		Sum
			one
			one
	AddOne
		one
	SetLocal
		hi
		AddOne
			Get
				hi
	Define
		AddTwo
		Number
		Parameter
			value
			Number
		Sum
			Get
				value
			one
			one
	SetLocal
		hi
		AddTwo
			Get
				hi
	Get
		hi""")
	print ast
	print ast.evaluate(root).value


if __name__ == "__main__":
	main()


# todo:
"""
you can't use anything as a value name after it has been defined as a definition or type
is that okay?


"""

