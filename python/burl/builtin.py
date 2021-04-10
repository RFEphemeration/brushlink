from burl.language import Context, Element, Parameter, Literal, Value
from burl.parser import ParseNode


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
			args = Builtin.unwrap_values(args)

		if self.use_context == Handling.CONTEXTUAL:
			result = self.func(context, *args)
		else:
			result = self.func(*args)

		if isinstance(result, Value):
			return result
		else:
			return Value(result, self.element_type)

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


def make_context(parent = None, values = None, definitions = None, types = None, evaluations = None, modules=None, temp_parent = None, load_core=True):
	context = Context(parent=temp_parent or parent, values=values, definitions=definitions, types=types, load_core=load_core)

	for evaluation in evaluations or []:
		parsed = ParseNode.parse(evaluation[1])
		element = parsed.evaluate(context).value
		if isinstance(element, Literal):
			element.value = evaluation[0]
		elif isinstance(element, Builtin):
			element.func = evaluation[0]

	if temp_parent:
		context.parent = parent

	return context


def literal(context, name, element_type):
	element = Literal(name, element_type, None)
	context.definitions[name] = element
	return element


def builtin(context, name, element_type, use_context, arg_handling, parameters):
	element = Builtin(name, element_type, None, use_context, arg_handling, parameters)
	context.definitions[name] = element
	return element


builtin_tools = make_context(
	load_core=False,
	types={
		'BuiltinContext',
		'ArgumentHandling',
	},
	definitions={
		'Literal': Builtin('Literal', 'Element', literal, Handling.CONTEXTUAL, Handling.UNWRAP, parameters=[
			Parameter('name', 'ValueName'),
			Parameter('element_type', 'Type'),
			]),
		'Builtin': Builtin('Builtin', 'Element', builtin, Handling.CONTEXTUAL, Handling.UNWRAP, parameters=[
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
