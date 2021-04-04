from command import *

# Testing

class Test():
	def __init__(self, name, expected_value, code):
		self.name = name
		self.code = code
		self.expected_value = expected_value
		self.run()

	def run(self):
		context = Context(root)
		ast = ParseNode.parse(self.code)
		result = ast.evaluate(context)
		if result.value == self.expected_value:
			print("Success: %s" % self.name)

def workspace():
	test = Context(root)
	test.definitions['AddOne'] = Definition('AddOne', 'Number', [
		Parameter('value', 'Number')
	], context=test, code="""
Sum 1 Get value
	""")
	
	ast = ParseNode.parse("""
Define AddTwo Number
	Parameter value Number
	Sum one one Get value""")
	print(ast)
	ast.evaluate(test)
	print(test.definitions.keys())
	
	ast = ParseNode.parse("""
SetLocal hi
	Sum 1 1
SetLocal hi
	AddOne Get hi
Define AddThree Number
	Parameter value Number
	Sum 3 Get value
SetLocal hi
	AddThree Get hi
If Some Compare 1 > 2 False
	Get hi
	Sum
		-1
		Get hi""")
	print(ast)
	print(ast.evaluate(test).value)


def main():
	# workspace()

	Test("Define and use", 2, """
Define AddOne Number
	Parameter value Number
	Sum 1 Get value
AddOne 1
	""")

	Test("Conditional", 1, """
If Some
		Compare 1 > 2
		True
	1
	0
	""")

	Test("Negative Numbers", 0, """
Sum 0 1 -1
	""")

	skiptest = """
ForEach
	first
	second
	third
	# disambiguates between repeatable first parameter and second parameter of same type
	| item
	SetLocal sum Get item Get sum
	"""


if __name__ == "__main__":
	main()
