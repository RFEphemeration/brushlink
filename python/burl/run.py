from burl.language import Context, EvaluationError
from burl.parser import ParseNode

def run_module(module, function, args = []):
	context = Context()
	ast = ParseNode.parse("""
LoadModule %s
%s %s""" % (module, function, " ".join(args)))
	try:
		result = ast.evaluate(context)
		print(result)
	except EvaluationError as e:
		print("EvaluationError: %s" % (e.__str__()))
