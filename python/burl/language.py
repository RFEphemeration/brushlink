import importlib


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

		success = self.try_append_to_mapped_arguments(arg)
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

	def try_append_to_mapped_arguments(self, next_argument):
		""" using True, False as Accept, Continue. exceptions are errors """
		param_index = len(self.mapped_arguments)
		if self.mapped_arguments and isinstance(self.mapped_arguments[-1], list):
			param_index -= 1

		for param_index in range(param_index, len(self.element.parameters)):
			param = self.element.parameters[param_index]
			# rmf todo: how to deal with unbound names here?
			if param.accepts(next_argument.element.element_type):
				if not param.repeatable:
					self.mapped_arguments.append(next_argument)
				else:
					if param_index < len(self.mapped_arguments):
						self.mapped_arguments[param_index].append(next_argument)
					else:
						self.mapped_arguments.append([next_argument])
				return True
			if param.required:
				raise EvaluationError("element %s parameter %s has an argument of incorrect type. expects %s, got %s." % (
						self.element.name, param.name, param.element_type,
						next_argument.element.element_type))
			else:
				if param.repeatable:
					self.mapped_arguments.append([])
				else:
					# rmf todo: should we evaluate default values yet?
					self.mapped_arguments.append(None)

		# we may have modified mapped_arguments by filling in default values
		# this should be alright, but will need updating with an undo feature
		return False


class Module:
	def __init__(self, name, context, value = Value(None, 'NoneType')):
		self.name = name
		self.context = context
		self.value = value


class ModuleDictionary:
	__instance = None
	__builtin_modules = {'core', 'collections', 'composition'}

	@staticmethod
	def instance():
		if not ModuleDictionary.__instance:
			ModuleDictionary.__instance = ModuleDictionary()
		return ModuleDictionary.__instance

	def __init__(self):
		self.modules = {}

	def get_module(self, path):
		if path in self.modules:
			return self.modules[path]
		elif path in ModuleDictionary.__builtin_modules:
			# try:
			importlib.import_module("burl." + path)
			if path in self.modules:
				return self.modules[path]
			else:
				raise EvaluationError("Importing builtin %s did not add a module" % path)
			""" # commenting this out for now because it makes debugging module failures harder
			except Exception as e:
				raise EvaluationError("Error while importing builtin module %s" % path)
			"""
		return None

	def add_module(self, module):
		self.modules[module.name] = module


# Context and builtins
class Context:
	def __init__(self, parent=None, values=None, definitions=None, types=None, load_core=None):
		if load_core is None:
			load_core = parent is None
		self.parent = parent
		self.values = values or {}
		self.definitions = definitions or {}
		self.types = types or set()
		self.module_references = {}
		if load_core:
			self.module_references['core'] = ModuleDictionary.instance().get_module('core')

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

