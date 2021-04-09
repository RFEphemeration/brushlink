import os

# Elements
class EvaluationError(Exception):
	pass


class Value(tuple):
	def __new__(cls, value, element_type):
		if isinstance(value, Value):
			raise EvaluationError("value is being nested, was %s, adding type %s" % (value, element_type))
		return tuple.__new__(cls, (value, element_type))

	@property
	def value(self):
		return self[0]

	@property
	def element_type(self):
		return self[1]

	def __str__(self):
		if self.value is None:
			return self.element_type + ": None"
		else:
			return self.element_type + ": " +str(self.value)

	def __repr__(self):
		return self.__str__()

	# not sure if these are necessary, or if inheriting from tuple is sufficient

	def __setattr__(self, *ignored):
		raise NotImplementedError

	def __delattr__(self, *ignored):
		raise NotImplementedError


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
				if isinstance(param.default_value, EvalNode):
					unevaluated_arguments[param_index].append(param.default_value)
				else:
					raise EvaluationError("element %s parameter %s is not an EvalNode, has value %s" % (self.name, param.name, str(param.default_value)))
			elif unevaluated_arguments[param_index] is None and (not param.repeatable) and param.default_value:
				if isinstance(param.default_value, EvalNode):
					unevaluated_arguments[param_index] = param.default_value
				else:
					raise EvaluationError("element %s parameter %s is not an EvalNode, has value %s" % (self.name, param.name, str(param.default_value)))

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
				# this is inefficient when might be passed in an element defined literal anyway
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

	def as_eval_node(self):
		return EvalNode(self, [])


class Handling:
	# argument handling
	LAZY = "Lazy"
	EAGER = "Eager"
	UNWRAP = "Unwrap"

	# use context
	STANDALONE = "Standalone"
	CONTEXTUAL = "Contextual"


class Builtin(Element):
	def __init__(self, name, element_type, func, use_context=Handling.CONTEXTUAL, handling=Handling.EAGER, parameters=None):
		Element.__init__(self, name, element_type, parameters)
		self.func = func
		self.use_context = use_context
		self.handling = handling

	def evaluate(self, context, unevaluated_arguments):
		if self.handling == Handling.LAZY:
			args = unevaluated_arguments
		else:
			args = self.evaluate_arguments(context, unevaluated_arguments)

		if self.handling == Handling.UNWRAP:
			args = Element.unwrap_values(args)

		if self.use_context == Handling.CONTEXTUAL:
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

		root_node = ParseNode("RootSequence", -1)
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
	# consider using an immutable data structure like Tuple(element, frozenmap)
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
					if isinstance(root_node.element, UnboundSymbol):
						raise EvaluationError("Failed to append argument %s to unbound symbol %s. Are you missing a module load?" % (name, root_node.element.name))
					else:
						raise EvaluationError("Failed to append argument %s to %s" % (name, root_node.element.name))

		last_node = root_node.get_last_argument()
		if last_node == root_node and parse_node.end_children:
			raise EvaluationError("Element %s has end children but only one element on the line" % root_node.element.name)

		for child in parse_node.end_children:
			child_node = EvalNode.from_parse_tree(context, child)
			success = last_node.element.try_append_to_mapped_arguments(last_node.mapped_arguments, child_node)
			if not success:
				raise EvaluationError("incorrect argument %s for %s" % (child_node.element.name, last_node.element.name))

		for child in parse_node.children:
			child_node = EvalNode.from_parse_tree(context, child)
			success = root_node.element.try_append_to_mapped_arguments(root_node.mapped_arguments, child_node)
			if not success:
				raise EvaluationError("incorrect argument %s for %s" % (child_node.element.name, last_node.element.name))
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


class Module:
	def __init__(self, name, context = None, code = None):
		self.name = name
		if not code and not Context:
			raise EvaluationError("Modules require either an existing context or code to evaluate, or both")
		self.context = context or Context()
		if code:
			parsed = ParseNode.parse(code)
			self.value = parsed.evaluate(self.context)
		else:
			self.value = Value(None, 'NoneType')


core = None

class ModuleDictionary:
	__instance = None

	@staticmethod
	def getInstance():
		if not ModuleDictionary.__instance:
			ModuleDictionary.__instance = ModuleDictionary()
		return ModuleDictionary.__instance

	def __init__(self, modules = None):
		if modules is not None:
			self.modules = modules
		elif core is not None:
			self.modules = {'core': Module('core', context = core)}
		else:
			self.modules = {}

	def load(self, path):
		if path in self.modules:
			return self.modules[path]
		try:
			with open(path) as f:
				self.modules[path] = Module(path, code = f.read())
				return self.modules[path]
		except (OSError, FileNotFoundError):
			raise EvaluationError("Could not find module with path " + path)
		except EvaluationError as e:
			raise EvaluationError("Parsing module failed at path " + path + "\n" + e.__str__())

# Context and builtins
class Context:
	def __init__(self, parent = None, values = None, definitions = None, types = None, evaluations = None, modules = [], temp_modules = [], temp_parent = None):
		self.parent = temp_parent or parent
		self.values = values or {}
		self.definitions = definitions or {}
		self.types = types or set()
		self.module_references = {}

		if self.parent is None and 'core' in ModuleDictionary.getInstance().modules:
			self.module_references['core'] = ModuleDictionary.getInstance().modules['core']

		for module in temp_modules:
			self.module_references[module] = ModuleDictionary.getInstance().load(module)

		for module in modules:
			self.module_references[module] = ModuleDictionary.getInstance().load(module)

		for evaluation in evaluations or []:
			parsed = ParseNode.parse(evaluation[1])
			element = parsed.evaluate(self).value
			if isinstance(element, Literal):
				element.value = evaluation[0]
			elif isinstance(element, Builtin):
				element.func = evaluation[0]

		for module in temp_modules:
			del self.module_references[module]

		if temp_parent:
			self.parent = parent

	# internal helpers

	def merge(self, context):
		self.values = self.values | context.values
		self.definitions = self.definitions | context.definitions
		self.types = self.types | context.types
		self.module_references = self.module_references | context.module_references

	def navigate_parents_and_modules(self, func, checked_modules = None):
		checked_modules = checked_modules or set()
		context = self
		while context:
			result = func(context)
			if result:
				return result
			else:
				# should we do modules of only ourselves, or all parents, too?
				# what about modules of modules?
				for loaded_name in context.module_references:
					module = context.module_references[loaded_name]
					if module.name in checked_modules:
						continue
					checked_modules.add(module.name)
					result = module.context.navigate_parents_and_modules(func, checked_modules)
					if result:
						return result
				context = context.parent
		return None

	def get_definition(self, name):
		value = self.navigate_parents_and_modules(lambda context : context.definitions.get(name, None))
		if value:
			return value
		raise EvaluationError("get failed, no definition found with name %s" % (name))

	def is_known_type(self, name):
		return self.navigate_parents_and_modules(lambda context : name in context.types)

	def get_symbol_type(self, name):
		def get_type(context):
			if name in context.types:
				return 'Type'
			elif name in context.definitions:
				return 'Element'
			elif name in context.values:
				return 'ValueName'
			else:
				return None
		symbol_type = self.navigate_parents_and_modules(get_type);
		if symbol_type:
			return symbol_type
		else:
			# assume unrecognized symbols are new value names, does this hold?
			return 'ValueName'

	# builtin functions

	def literal(self, name, element_type):
		element = Literal(name, element_type, None)
		self.definitions[name] = element
		return element

	def builtin(self, name, element_type, use_context, arg_handling, parameters):
		element = Builtin(name, element_type, None, use_context, arg_handling, parameters)
		self.definitions[name] = element
		return element

	def define(self, name, element_type, parameters, evaluator):
		eval_name = name.evaluate(self).value
		eval_type = element_type.evaluate(self).value
		eval_params = []
		for param in parameters:
			eval_params.append(param.evaluate(self).value)
		root_node = EvalNode(self.get_definition('Sequence'), [evaluator])
		element = Definition(eval_name, eval_type, eval_params, evaluator=root_node)
		self.definitions[eval_name] = element
		# rmf todo: should this return the definition? or just nothing
		return element

	def load_module(self, path, name):
		if not name:
			name = os.path.basename(path)
			if name.endswith('.burl'):
				name = name[:-5]
		name = name or os.path.splitext()
		if name in self.module_references:
			raise EvaluationError("Cannot load module with name " + name + ", another module with this name already exists")
		self.module_references[name] = ModuleDictionary.getInstance().load(path)
		return self.module_references[name].value

	def set(self, name, value):
		self.values[name.value] = value
		return value

	def set_global(self, name, value):
		context = self
		while context.parent:
			context = context.parent
		context.values[name.value] = value
		return value

	def get(self, name):
		if name.value in self.values:
			return self.values[name.value]
		else:
			raise EvaluationError("get failed, no value found with name %s" % (name.value))	

	def get_global(self, name):
		context = self
		while context.parent:
			context = context.parent
		if name.value in context.values:
			return context.values[name.value]
		else:
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

	# composition tools

	def all_types(self):
		types = set()
		def merge_types(context):
			nonlocal types
			for t in context.types:
				types.add(Value(t, 'Type'))
			return False # navigate to full depth
		self.navigate_parents_and_modules(merge_types)
		return frozenset(types)

	def definitions_of_type(self, element_type):
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
		self.navigate_parents_and_modules(merge_definitions)
		return frozenset(definitions)

	def values_of_type(self, element_type):
		values = {}
		def merge_values(context):
			nonlocal values
			# what to do about name conflicts?
			for name in context.values:
				# do we want elements that a parameter of type would accept?
				if context.values[name].element_type == element_type.value:
					values.add(Value(name, 'ValueName'))
			return False # navigate to full depth
		self.navigate_parents_and_modules(merge_values)
		return frozenset(values)

	def evaluate(self, node):
		if node.element_type == 'EvaluationError':
			return node
		try:
			value = node.value.evaluate(self)
			if isinstance(value, Value):
				return value
			else:
				return Value(value, node.value.element.element_type)
		except EvaluationError as e:
			return Value(e, 'EvaluationError')

	def append_argument(self, tree, arg):
		# this modifies the argument, which is not ideal
		try:
			success = tree.value.append_argument(arg.value)
			if success:
				return Value(tree.value, 'EvalNode')
			else:
				return Value('Could not append argument %s to %s' % (
					arg.value.element.name, tree.value.element.name), 'EvaluationError')
		except EvaluationError as e:
			return Value(e, 'EvaluationError')


def parameter(name, element_type, repeatable, default_value):
	default_node = None
	if default_value.value is None:
		default_node = None
	elif isinstance(default_value.value, EvalNode):
		default_node = default_value.value
	else:
		default_node = EvalNode(Literal(
			str(default_value.value),
			default_value.element_type,
			default_value.value), [])
	return Parameter(name.value, element_type.value, default_value=default_node, repeatable=repeatable.value)


def quote(node):
	return Value(node, 'EvalNode')


def element_sum(operand, operands):
	return sum(operands, operand)


def sequence(expressions):	
	return expressions[-1]


class Comparison:
	EQ = "Equal"
	NE = "NotEqual"
	LT = "Lesser"
	GT = "Greater"
	LT_EQ = "LesserOrEqual"
	GT_EQ = "GreaterOrEqual"


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

core = Context(types={
		'Any',
		'NoneType',
		'Type',
		'ValueName',
		'Boolean',
		'ParameterType',
		'Element',
		# this is only for quote, I'd like to move those things to reflection
		# consider instead a nested None literal
		'EvalNode', 
	},
	definitions = {
		'RootSequence': Builtin('RootSequence', 'Any', sequence, Handling.STANDALONE, parameters=[
			Parameter('expressions', 'Any', repeatable=True)
		]),
		'Sequence': Builtin('Sequence', 'Any', sequence, Handling.STANDALONE, parameters=[
			Parameter('expressions', 'Any', repeatable=True)
		]),
		'Parameter': Builtin('Parameter', 'ParameterType', parameter, Handling.STANDALONE, Handling.EAGER, [
			# rmf todo: default name based on index
			Parameter('name', 'ValueName'),
			# rmf todo: type disambiguation from definitions
			Parameter('element_type', 'Type'),
			Parameter('repeatable', 'Boolean', default_value=Literal('False', 'Boolean', False).as_eval_node()),
			# rmf todo: generics/polymorphism, this should match element_type Optional<Element<$T>>
			# can this just be required=false? what does default value of None do here?
			Parameter('default_value', 'Any', default_value=Literal('None', 'NoneType', None).as_eval_node()),
		]),
		'Define': Builtin('Define', 'Element', Context.define, handling=Handling.LAZY, parameters=[
			Parameter('name', 'ValueName'),
			Parameter('element_type', 'Type'),
			Parameter('parameters', 'ParameterType', required=False, repeatable=True),
			# rmf todo: generics/polymorphism, this should match element_type
			Parameter('code', 'Any', repeatable=True),
		]),
		'Quote': Builtin('Quote', 'EvalNode', quote, Handling.STANDALONE, Handling.LAZY, [
			Parameter('element', 'Any'),
		]),
		'LoadModule': Builtin('LoadModule', 'Any', Context.load_module, handling=Handling.UNWRAP, parameters=[
			# maybe path should be a string literal
			# and maybe you just need one of the two
			Parameter('path', 'ValueName'),
			Parameter('name', 'ValueName', default_value=Literal('None', 'NoneType', None).as_eval_node()),
		]),
	},
	# no evaluations on first pass because we don't have builtin tools yet
	evaluations = []
)

ModuleDictionary.getInstance().modules['core'] = Module('core', context=core)

builtin_tools = Context(
	types={
		'BuiltinContext',
		'ArgumentHandling',
	},
	definitions={
		'Literal': Builtin('Literal', 'Element', Context.literal, handling=Handling.UNWRAP, parameters=[
			Parameter('name', 'ValueName'),
			Parameter('element_type', 'Type'),
			]),
		'Builtin': Builtin('Builtin', 'Element', Context.builtin, handling=Handling.UNWRAP, parameters=[
			Parameter('name', 'ValueName'),
			Parameter('element_type', 'Type'),
			Parameter('context', 'BuiltinContext', default_value=Literal('Contextual', 'BuiltinContext', Handling.STANDALONE).as_eval_node()),
			Parameter('handling', 'ArgumentHandling', default_value=Literal('Eager', 'ArgumentHandling', Handling.EAGER).as_eval_node()),
			Parameter('parameters', 'ParameterType', required=False, repeatable=True),
			]),
	},
	evaluations=[
		[Handling.STANDALONE, "Literal Standalone BuiltinContext"],
		[Handling.CONTEXTUAL, "Literal Contextual BuiltinContext"],
		[Handling.LAZY, "Literal Lazy ArgumentHandling"],
		[Handling.EAGER, "Literal Eager ArgumentHandling"],
		[Handling.UNWRAP, "Literal Unwrap ArgumentHandling"],
	],
)

# by using builtin tools as a parent, but not adding it to the module dictionary,
# and then merging the result, we can use builtin tools at init evaluation time without exposing it
# to the runtime
core.merge(Context(
	temp_parent=builtin_tools,
	types={
		'Number',
		'Comparison',
	},
	evaluations=[
		[None, "Literal None NoneType"],
		[True, "Literal True Boolean"],
		[False, "Literal False Boolean"],
		[Comparison.EQ, "Literal == Comparison"],
		[Comparison.NE, "Literal /= Comparison"],
		[Comparison.LT, "Literal < Comparison"],
		[Comparison.GT, "Literal > Comparison"],
		[Comparison.LT_EQ, "Literal <= Comparison"],
		[Comparison.GT_EQ, "Literal >= Comparison"],
		[Context.for_loop, """Builtin For Any Contextual Lazy 
	Parameter count Number
	Parameter value_name ValueName Quote None
	Parameter expression Any"""],
		[Context.for_each_loop, """Builtin ForEach Any Contextual Lazy
	Parameter value_name ValueName
	Parameter values Any True
	Parameter expression Any"""],
		[Context.while_loop, """Builtin While Any Contextual Lazy
	Parameter condition Boolean
	Parameter expression Any"""],
		[Context.if_branch, """Builtin If Any Contextual Lazy
	Parameter condition Boolean
	Parameter consequent Any
	Parameter alternative Any Quote None"""],
		[Context.set, """Builtin Set Any Contextual
	Parameter name ValueName
	Parameter value Any
		"""],
		[Context.set_global, """Builtin SetGlobal Any Contextual
	Parameter name ValueName
	Parameter value Any
		"""],
		[Context.get, "Builtin Get Any Contextual Parameter name ValueName"],
		[Context.get_global, "Builtin GetGlobal Any Contextual Parameter name ValueName"],
		[Context.logical_not, "Builtin Not Boolean Contextual Parameter expression Boolean"],
		[Context.logical_all, "Builtin All Boolean Contextual Lazy Parameter expressions Boolean True"],
		[Context.logical_any, "Builtin Some Boolean Contextual Lazy Parameter expressions Boolean True"],
		[sum, "Builtin Sum Number Unwrap Parameter operands Number True"],
		[compare, """Builtin Compare Boolean Unwrap
	Parameter left Number
	Parameter comparison Comparison
	Parameter right Number
		"""],
	],
))

ModuleDictionary.getInstance().modules['collections'] = Module('collections', context=Context(
	temp_parent=builtin_tools,
	types={
		'HashSet',
	},
	evaluations=[
		[frozenset(), "Literal HashSet.Empty HashSet"],
		[frozenset, "Builtin HashSet.Make HashSet Parameter values Any True"],
		[len, "Builtin HashSet.Count Number Unwrap Parameter set HashSet"],
		[frozenset.union, """Builtin HashSet.Union HashSet Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
		[frozenset.intersection, """Builtin HashSet.Intersect HashSet Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
		[frozenset.difference, """Builtin HashSet.Difference HashSet Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
		[frozenset.symmetric_difference, """Builtin HashSet.SymDifference HashSet Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
		[frozenset.isdisjoint, """Builtin HashSet.Disjoint Boolean Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
		[frozenset.issubset, """Builtin HashSet.Subset Boolean Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
		[frozenset.issuperset, """Builtin HashSet.Superset Boolean Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
	],
))


ModuleDictionary.getInstance().modules['composition'] = Module('composition', context=Context(
	temp_parent=builtin_tools,
	modules=['collections'],
	types={
		'EvaluationError',
	},
	evaluations=[
		[Context.all_types, "Builtin AllTypes HashSet Contextual"],
		[Context.definitions_of_type, "Builtin DefinitionsOfType HashSet Contextual Parameter type Type"],
		[Context.values_of_type, "Builtin ValuesOfType HashSet Contextual Parameter type Type"],
		[Context.evaluate, "Builtin Evaluate Any Contextual Parameter node EvalNode"],
		[Context.append_argument, """Builtin AppendArgument EvalNode Contextual
	Parameter root_node EvalNode
	Parameter argument EvalNode"""],
	],
))
