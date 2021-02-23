

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
		#print "parameter: %s, type %s, argument: %s" % (self.name, self.element_type, element_type)
		return self.element_type == element_type or self.element_type == 'any'

class Context:
	def __init__(self, parent, definitions):
		self.parent = parent
		self.definitions = definitions
		self.values = {}

	def set_local(self, name, value):
		self.values[name] = value
		#print "post-set: "
		#print self.values
		return value

	def set_global(self, name, value):
		context = self
		while context.parent:
			context = context.parent
		context.values[name] = value
		return value

	def get(self, name):
		#print "trying-get: "
		context = self
		while context:
			#print context.values
			if name in context.values:
				return context.values[name]
			else:
				context = context.parent
		raise EvaluationError("get failed, no variable found with name %s" % (name))


class Element:
	def __init__(self, name, element_type, implementation, parameters):
		self.name = name
		self.element_type = element_type
		self.implementation = implementation
		self.parameters = parameters

	def evaluate(self, context, arguments):
		# todo: get default arguments for parameters and such
		args = []
		param_repeatable_found = False
		param_index = 0
		for arg in arguments:
			if param_index > len(self.parameters):
				raise EvaluationError("element %s has too many arguments." % (self.name))
			while True:
				param = self.parameters[param_index]
				if param.accepts(arg.element_type):
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
					# default values shouldn't require any arguments
					args.append(param.default_value.evaluate(context, []))
					param_index += 1
					param_repeatable_found = False
				else:
					raise EvaluationError("element %s has an argument of incorrect type. expects %s, got %s."  % (self.name, param.element_type, arg.element_type))
		if len(args) != len(self.parameters):
			raise EvaluationError("element %s has an incorrect number of parameters" % (self.name))
		#print arguments
		#print args
		value = self.implementation.evaluate(context, args)
		if type(value) is Value:
			return value
		else:
			return Value(value, self.element_type);

class Implementation:
	def evaluate(self, context, arguments):
		pass

class Literal(Implementation):
	def __init__(self, value):
		self.value = value

	def evaluate(self, context, arguments):
		return self.value

class Builtin(Implementation):
	def __init__(self, func):
		self.func = func

	def evaluate(self, context, arguments):
		arg_values = []
		for arg in arguments:
			if type(arg) is list:
				repeatable = []
				for repeat in arg:
					repeatable.append(repeat.value)
				arg_values.append(repeatable)
			else:
				arg_values.append(arg.value)
		return self.func(*arg_values)

class ContextBuiltin(Implementation):
	def __init__(self, func):
		self.func = func

	def evaluate(self, context, arguments):
		arg_values = []
		for arg in arguments:
			if type(arg) is list:
				repeatable = []
				for repeat in arg:
					repeatable.append(repeat.value)
				arg_values.append(repeatable)
			else:
				arg_values.append(arg.value)
		#print arg_values
		#print arguments
		return self.func(context, *arg_values)

class Definition(Implementation):
	def __init__(self, element):
		self.element = element

	def evaluate(self, context, arguments):
		self.element.evaluate(arguments);

def element_sum(operand, operands):
	return sum(operands, operand)

def sequence(children):
	return children[-1]

root = Context(None, {
	'sequence' : Element('sequence', 'any', Builtin(sequence), [
		Parameter('children', 'any', repeatable=True)
	]),
	'set_local': Element('set_local', 'none', ContextBuiltin(Context.set_local), [
		Parameter('name', 'name'),
		Parameter('value', 'any'),
	]),
	'set_global': Element('set_global', 'none', ContextBuiltin(Context.set_global), [
		Parameter('name', 'name'),
		Parameter('value', 'any'),
	]),
	'get': Element('get', 'any', ContextBuiltin(Context.get), [
		Parameter('name', 'name'),
	]),
	'one' : Element('one', 'number', Literal(1), []),
	'sum': Element('sum', 'number', Builtin(sum), [
		Parameter('operands', 'number', repeatable=True)
	]),
})

class Node:
	def __init__(self, contents, indentation, parent = None, children = None):
		self.contents = contents
		self.indentation = indentation
		self.parent = parent
		if children:
			self.children = children
		else:
			self.children = []

	def __str__(self, indentation = 0):
		out = ("   " * indentation) + self.contents
		for child in self.children:
			out += "\n" + child.__str__(indentation + 1)
		return out

	def __repr__(self):
		return self.__str__()

	def evaluate(self, context):
		#print "evaluating: " + self.contents
		value = Value(None, 'none')
		args = []
		for child in self.children:
			args.append(child.evaluate(context))
		# todo: more than one element per node, implicit children
		if self.contents in context.definitions:
			value = context.definitions[self.contents].evaluate(context, args)
		else:
			#for now just assume this is a variable name
			value = Value(self.contents, 'name')
			if args:
				raise ("unrecognized element %s was treated like a value name but has arguments" % (self.contets))
		#print "got: " + str(value.value)
		return value

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
			nodes.append(Node(contents, indentation))

	root = Node("sequence", 0)
	for node in nodes:
		if node.contents == "":
			continue
		current = root
		while current.children and node.indentation > (current.indentation + 1):
			current = current.children[-1]
		current.children.append(node)
		node.parent = current

	return root


def main():
	print root.definitions.keys()
	ast = parse ("""
	sum
		one
		one""")

	print ast
	print ast.evaluate(root).value

	ast = parse ("""
	set_local
		hi
		sum
			one
			one
	get
		hi""")

	#print ast.children[-1].children

	print ast
	print ast.evaluate(root).value

if __name__ == "__main__":
	main()