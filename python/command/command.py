

class EvaluationError(Exception):
	pass

class Value:
	def __init__(self, value, element_type):
		self.value = value
		self.element_type = element_type

class Parameter:
	def __init__(self, name, element_type, default_value = None, repeatable = False):
		self.name = name
		self.element_type = element_type
		self.default_value = default_value
		self.repeatable = repeatable

class Element:
	def __init__(self, name, element_type, parameters, implementation):
		self.name = name
		self.element_type = element_type
		self.parameters = parameters
		self.implementation = implementation

	def evaluate(self, arguments):
		# todo: get default arguments for parameters and such
		args = []
		param_repeatable_found = False
		param_index = 0
		for arg in arguments:
			if param_index > len(self.parameters):
				raise EvaluationError("element %s has too many arguments." % (self.name))
			while True:
				param = self.parameters[param_index]
				if param.element_type == arg.element_type:
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
					args.append(param.default_value.evaluate([]))
					param_index += 1
					param_repeatable_found = False
				else:
					raise EvaluationError("element %s has an argument of incorrect type. expects %s, got %s."  % (self.name, param.element_type, arg.element_type))

		return Value(self.implementation.evaluate(args), self.element_type);

class Implementation:
	def evaluate(self, arguments):
		pass

class Literal(Implementation):
	def __init__(self, value):
		self.value = value

	def evaluate(self, arguments):
		return self.value

class Builtin(Implementation):
	def __init__(self, func):
		self.func = func

	def evaluate(self, arguments):
		args = []
		for arg in arguments:
			if type(arg) is list:
				repeatable = []
				for repeat in arg:
					repeatable.append(repeat.value)
				args.append(repeatable)
			else:
				args.append(arg.value)
		return self.func(*args)

class Defined(Implementation):
	def __init__(self, element):
		self.element = element

	def evaluate(self, arguments):
		self.element.evaluate(arguments);

def element_sum(operand, operands):
	return sum(operands, operand)

definitions = {
	'one' : Element('one', 'number', [], Literal(1)),
	'sum': Element('sum', 'number', [
		Parameter('operands', 'number', repeatable=True)
		], Builtin(sum))
}

class Node:
	def __init__(self, contents, indentation, parent = None, children = None):
		self.contents = contents
		self.indentation = indentation
		self.parent = parent
		if children:
			self.children = children
		else:
			self.children = []

	def __str__(self):
		out = ("\t" * self.indentation) + self.contents + "\n"
		for child in self.children:
			out += child.__str__()
		return out

	def output(self):
		line = ("\t" * self.indentation) + self.contents
		#print line
		for child in self.children:
			child.output();

def parse(code):
	nodes = []
	lines = code.splitlines()
	for line in lines:
		contents = line.lstrip('\t')
		indentation = len(line) - len(contents)
		if nodes and indentation >= nodes[-1].indentation + 2:
			# this is a continuation of the previous line, not a new node
			nodes[-1].contents += " " + contents
		else:
			nodes.append(Node(contents, indentation))

	root = Node("", -1)
	for node in nodes:
		current = root
		while current.children and indentation > current.indentation + 1:
			current = current.children[-1]
		current.children.append(node)
		node.parent = current

	return root



def main():
	print definitions['one'].evaluate([]).value
	print definitions['sum'].evaluate([Value(1, 'number'), Value(1, 'number')]).value

	ast = parse ("""sum
	one
	one""")

	print ast

	#print ast.contents
	#print len(ast.children)
	#print ast.children[0].contents
	#print ast.children[1].contents

if __name__ == "__main__":
	main()

