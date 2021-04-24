from burl.language import Context, EvaluationError
from burl.parser import ParseNode

def run_module(module, function, args = []):
	context = Context()
	ast = ParseNode.parse("""
LoadModule %s
%s %s""" % (module, function, " ".join(args)))
	try:
		result = ast.evaluate(context)
		if result.element_type == "ValueName" and result.value == function:
			# will throw an error if the function doesn't exist
			context.get_definition(function)
		print(result)
	except EvaluationError as e:
		print("EvaluationError: %s" % (e.__str__()))
