
# Elements


class EvaluationError(Exception):
	pass

class Value:
	def __init__(self, value, element_type):
		self.value = value
		self.element_type = element_type

	def __str__(self):
		return str(self.value) + ": " + self.element_type

	def __repr__(self):
		return self.__str__()

class Parameter:
	def __init__(self, name, element_type, default_value = None, repeatable = False):
		self.name = name
		self.element_type = element_type
		self.default_value = default_value
		self.repeatable = repeatable

	def accepts(self, element_type):
		return self.element_type == element_type or self.element_type == 'any'


class Element:
	def __init__(self, name, element_type, parameters = None):
		self.name = name
		self.element_type = element_type
		self.parameters = parameters if parameters is not None else []

	def evaluate(self, context, unevaluated_arguments):
		pass

	def match_args_to_params(self, context, unevaluated_arguments):
		args = []
		param_repeatable_found = False
		param_index = 0
		for arg in unevaluated_arguments:
			if param_index > len(self.parameters):
				raise EvaluationError("element %s has too many arguments." % (self.name))
			while True:
				param = self.parameters[param_index]
				if param.accepts(arg.element.element_type):
					if param.repeatable:
						if param_repeatable_found:
							args[-1].append(arg)
						else:
							args.append([arg])
							param_repeatable_found = True
					else:
						args.append(arg)
						param_index += 1
					break
				elif param.repeatable and param_repeatable_found:
					# this arg isn't for this repeatable param
					# but we already have a value for it, so no need to add default
					param_index += 1
					param_repeatable_found = False
				elif param.default_value:
					default = context.get_definition(param.default_value)
					# default values don't require any arguments
					args.append(EvalNode(default, []))
					param_index += 1
					param_repeatable_found = False
				else:
					raise EvaluationError("element %s has an argument of incorrect type. expects %s, got %s."  % (self.name, param.element_type, arg.element_type))
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
	def strip_values(evaluated_arguments):
		args = []
		for arg in evaluated_arguments:
			if type(arg) is list:
				repeatable = []
				for repeat in arg:
					repeatable.append(repeat.value)
				args.append(repeatable)
			else:
				args.append(arg.value)
		return args

class Literal(Element):
	def __init__(self, name, element_type, value):
		Element.__init__(self, name, element_type)
		self.value = value

	def evaluate(self, context, unevaluated_arguments):
		if unevaluated_arguments:
			raise EvaluationError("literal %s shouldn't have any arguments." % (self.name))
		if type(self.value) is Value:
			return self.value
		else:
			return Value(self.value, self.element_type);

class Builtin(Element):
	def __init__(self, name, element_type, func, parameters):
		Element.__init__(self, name, element_type, parameters)
		self.func = func

	def evaluate(self, context, unevaluated_arguments):
		value_args = self.evaluate_arguments(context, unevaluated_arguments)
		args = Element.strip_values(value_args)
		result = self.func(*args)
		if type(result) is Value:
			return result
		else:
			return Value(result, self.element_type);

class ContextBuiltin(Element):
	def __init__(self, name, element_type, func, parameters):
		Element.__init__(self, name, element_type, parameters)
		self.func = func

	def evaluate(self, context, unevaluated_arguments):
		value_args = self.evaluate_arguments(context, unevaluated_arguments)
		args = Element.strip_values(value_args)
		result = self.func(context, *args)
		if type(result) is Value:
			return result
		else:
			return Value(result, self.element_type);

class Definition(Element):
	def __init__(self, name, element_type, parameters, evaluator):
		Element.__init__(self, name, element_type, parameters)
		self.evaluator = evaluator

	def evaluate(self, context, unevaluated_arguments):
		args = self.evaluate_arguments(context, unevaluated_arguments)
		values = {}
		for param, arg in zip(self.parameters, args):
			values[param.name] = arg
		child_context = Context(context, values=values)
		self.evaluator.evaluate(child_context);

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

		root = ParseNode("sequence", 0)
		for node in nodes:
			if node.contents == "":
				continue
			current = root
			while current.children and node.indentation > (current.indentation + 1):
				current = current.children[-1]
			current.children.append(node)
			node.parent = current

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
			# assume this is a value name
			# todo: it could also be something that is not yet defined
			# todo: it feels weird to use literals for value names
			element = Literal(parse_node.contents, 'name', parse_node.contents)
		node = EvalNode(element)
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

	# builtin functions

	def set_local(self, name, value):
		self.values[name] = value
		return value

	def set_global(self, name, value):
		context = self
		while context.parent:
			context = context.parent
		context.values[name] = value
		return value

	def get(self, name):
		context = self
		while context:
			if name in context.values:
				return context.values[name]
			else:
				context = context.parent
		raise EvaluationError("get failed, no value found with name %s" % (name))

def element_sum(operand, operands):
	return sum(operands, operand)

def sequence(children):
	return children[-1]

root = Context(None, definitions = {
	'sequence' : Builtin('sequence', 'any', sequence, [
		Parameter('children', 'any', repeatable=True)
	]),
	'set_local': ContextBuiltin('set_local', 'none', Context.set_local, [
		Parameter('name', 'name'),
		Parameter('value', 'any'),
	]),
	'set_global': ContextBuiltin('set_global', 'none', Context.set_global, [
		Parameter('name', 'name'),
		Parameter('value', 'any'),
	]),
	'get': ContextBuiltin('get', 'any', Context.get, [
		Parameter('name', 'name'),
	]),
	'one' : Literal('one', 'number', 1),
	'sum': Builtin('sum', 'number', sum, [
		Parameter('operands', 'number', repeatable=True)
	]),
})

# Testing

def main():
	print root.definitions.keys()
	ast = ParseNode.parse ("""
	set_local
		hi
		sum
			one
			one
	get
		hi""")

	print ast
	print ast.evaluate(root).value

if __name__ == "__main__":
	main()
