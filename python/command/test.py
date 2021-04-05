from command import *

# Testing

class Test():
	total = 0
	success = 0
	wrong = 0
	error = 0
	def __init__(self, name, expected_value, code):
		self.name = name
		self.code = code
		self.expected_value = expected_value
		self.run()

	def run(self):
		Test.total += 1
		context = Context()
		ast = ParseNode.parse(self.code)
		try:
			result = ast.evaluate(context)
			if result.value == self.expected_value:
				print("Success: %s" % self.name)
				Test.success += 1
			else:
				print("  Wrong: %s, Expected: %s, Got: %s" % (self.name, str(self.expected_value), result.__str__()))
				Test.wrong += 1
		except EvaluationError as e:
			print("  Error: %s - %s" % (self.name, e.__str__()))
			Test.error += 1

def workspace():
	test = Context()
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
		False
		Compare 1 < 2
	1
	0
	""")

	Test("Negative Numbers", 0, """
Sum 0 1 -1
	""")

	Test("Load Module Value", 2, """
LoadModule ./command/test_data.burl
	""")

	Test("Load Module Use Defines", 3, """
LoadModule ./command/test_data.burl
Sum
	Two
	1
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

	if Test.success == Test.total:
		print("All %i tests passed" % Test.total)
	else:
		print("Failure: Total: %i, Success: %i, Wrong: %i, Error: %i" % (Test.total, Test.success, Test.wrong, Test.error))

if __name__ == "__main__":
	main()
