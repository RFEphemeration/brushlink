from command import *

# Testing


def main():
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
	If Any Compare 1 > 2 False
		Get hi
		Sum
			-1
			Get hi""")
	print(ast)
	print(ast.evaluate(test).value)


if __name__ == "__main__":
	main()
