import os

from burl.language import Value, Parameter, Context, Module, ModuleDictionary, Literal, EvaluationError, EvalNode, Element
from burl.builtin import Builtin, builtin_tools, make_context, Handling
from burl.parser import ParseNode, Skip


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
		return self.evaluator.evaluate(child_context)

	@staticmethod
	def define(context, name, element_type, parameters, evaluator):
		eval_name = name.evaluate(context).value
		eval_type = element_type.evaluate(context).value
		eval_params = []
		for param in parameters:
			eval_params.append(param.evaluate(context).value)
		root_node = EvalNode(context.get_definition('Sequence'), [evaluator])
		element = Definition(eval_name, eval_type, eval_params, root_node)
		context.definitions[eval_name] = element
		# rmf todo: should this return the definition? or just nothing
		return element

	@staticmethod
	def make_lambda(context, parameters, evaluators):
		eval_params = []
		for param in parameters:
			eval_params.append(param.evaluate(context).value)
		element_type = evaluators[-1].element.element_type
		if len(evaluators) == 1:
			root_node = evaluators[0]
		else:
			root_node = EvalNode(context.get_definition('Sequence'), evaluators)

		return Definition("Lambda", element_type, eval_params, root_node)


def quote(node):
	return Value(node, 'EvalNode')


def evaluate_element(context, element, args):
	eval_element = element.evaluate(context).value
	return eval_element.evaluate(context, args)


def sequence(expressions):	
	return expressions[-1]


def parameter(name, element_type, repeatable, default_value):
	default_node = None
	if default_value.value is None:
		default_node = None
	elif isinstance(default_value.value, EvalNode):
		default_node = default_value.value
	else:
		default_node = EvalNode(Literal(
			str(default_value.value),
			default_value.element_type,
			default_value.value), [])
	return Parameter(name.value, element_type.value, default_value=default_node, repeatable=repeatable.value)


def load_module(context, path, name):
	if not name:
		name = os.path.basename(path)
		if name.endswith('.burl'):
			name = name[:-5]
	name = name or os.path.splitext()
	if name in context.module_references:
		raise EvaluationError("Cannot load module with name " + name + ", another module with this name already exists")

	module = ModuleDictionary.instance().get_module(path)
	if module:
		context.module_references[name] = module
		return module.value
	try:
		with open(path) as f:
			module_context = Context()
			value = ParseNode.parse(f.read()).evaluate(module_context)
			module = Module(path, context = module_context, value = value)
			ModuleDictionary.instance().add_module(module)
			context.module_references[path] = module
			return module.value
	except (OSError, FileNotFoundError):
		raise EvaluationError("Could not find module with path " + path)
	except EvaluationError as e:
		raise EvaluationError("Parsing module failed at path " + path + "\n" + e.__str__())


class Comparison:
	EQ = "Equal"
	NE = "NotEqual"
	LT = "Lesser"
	GT = "Greater"
	LT_EQ = "LesserOrEqual"
	GT_EQ = "GreaterOrEqual"


def compare(left, comparison, right):
	comparators = {
		Comparison.EQ: lambda x, y: x == y,
		Comparison.NE: lambda x, y: x != y,
		Comparison.LT: lambda x, y: x < y,
		Comparison.GT: lambda x, y: x > y,
		Comparison.LT_EQ: lambda x, y: x <= y,
		Comparison.GT_EQ: lambda x, y: x >= y
	}
	return comparators[comparison](left, right)


def for_loop(context, count, index_name, expression):
	value = Value(None, 'None')
	eval_index_name = index_name.evaluate(context).value
	for i in range(count.evaluate(context).value):
		if eval_index_name:
			context.set(context, eval_index_name, Value(i, 'Number'))
		value = expression.evaluate(context)
	return value


def for_each_loop(context, values, index_name, expression):
	eval_index_name = index_name.evaluate(context).value
	eval_values = []
	for value in values:
		eval_values.append(value.evaluate(context))

	ret = Value(None, 'None')
	for value in eval_values:
		context.set(context, eval_index_name, value)
		ret = value.evaluate(context)
	return ret


def while_loop(context, condition, expression):
	value = Value(None, 'None')
	while condition.evaluate(context).value:
		value = expression.evaluate(context)
	return value


def if_branch(context, condition, consequent, alternative):
	if condition.evaluate(context).value:
		return consequent.evaluate(context).value
	else:
		return alternative.evaluate(context).value


def logical_not(value):
	return Value(not value.value, 'Boolean')


def logical_all(context, expressions):
	for expr in expressions:
		if not expr.evaluate(context).value:
			return Value(False, 'Boolean')
	return Value(True, 'Boolean')


def logical_any(context, expressions):
	for expr in expressions:
		if expr.evaluate(context).value:
			return Value(True, 'Boolean')
	return Value(False, 'Boolean')


ModuleDictionary.instance().add_module(Module('core', context=make_context(
	load_core=False,
	temp_parent=builtin_tools,
	types={
		'Any',
		'NoneType',
		'Type',
		'ValueName',
		'Boolean',
		'Number',
		'Comparison',
		'ParameterType',
		'Element',
		'EvalNode', 
	},
	definitions={
		'RootSequence': Builtin('RootSequence', 'Any', sequence, Handling.STANDALONE, parameters=[
			Parameter('expressions', 'Any', repeatable=True)
		]),
		'Sequence': Builtin('Sequence', 'Any', sequence, Handling.STANDALONE, parameters=[
			Parameter('expressions', 'Any', repeatable=True)
		]),
		'Parameter': Builtin('Parameter', 'ParameterType', parameter, Handling.STANDALONE, Handling.EAGER, [
			# rmf todo: default name based on index
			Parameter('name', 'ValueName'),
			# rmf todo: type disambiguation from definitions
			Parameter('element_type', 'Type'),
			Parameter('repeatable', 'Boolean', default_value=Literal('False', 'Boolean', False).as_eval_node()),
			# rmf todo: generics/polymorphism, this should match element_type Optional<Element<$T>>
			# can this just be required=false? what does default value of None do here?
			Parameter('default_value', 'Any', default_value=Literal('None', 'NoneType', None).as_eval_node()),
		]),
		'Define': Builtin('Define', 'Element', Definition.define, handling=Handling.LAZY, parameters=[
			Parameter('name', 'ValueName'),
			Parameter('element_type', 'Type'),
			Parameter('parameters', 'ParameterType', required=False, repeatable=True),
			# rmf todo: generics/polymorphism, this should match element_type
			Parameter('code', 'Any', repeatable=True),
		]),	
		'LoadModule': Builtin('LoadModule', 'Any', load_module, handling=Handling.UNWRAP, parameters=[
			# maybe path should be a string literal
			# and maybe you just need one of the two
			Parameter('path', 'ValueName'),
			Parameter('name', 'ValueName', default_value=Literal('None', 'NoneType', None).as_eval_node()),
		]),
		'Skip': Skip(),
	},
	evaluations=[
		[None, "Literal None NoneType"],
		[Context.set, """Builtin Set Any Contextual
	Parameter name ValueName
	Parameter value Any
		"""],
		[Context.set_global, """Builtin SetGlobal Any Contextual
	Parameter name ValueName
	Parameter value Any
		"""],
		[quote, "Builtin Quote EvalNode Standalone Lazy Parameter element Any"],
		[Definition.make_lambda, """Builtin Lambda Element Contextual Lazy
	Parameter parameters ParameterType True
	Parameter evaluator Any True"""],
		[evaluate_element, """Builtin EvaluateElement Any Contextual Lazy
	Parameter element Element
	Parameter args Any True"""], #todo make arg types parameterize on the element's parameters
		[True, "Literal True Boolean"],
		[False, "Literal False Boolean"],
		[Context.get, "Builtin Get Any Contextual Parameter name ValueName"],
		[Context.get_global, "Builtin GetGlobal Any Contextual Parameter name ValueName"],
		[Comparison.EQ, "Literal == Comparison"],
		[Comparison.NE, "Literal /= Comparison"],
		[Comparison.LT, "Literal < Comparison"],
		[Comparison.GT, "Literal > Comparison"],
		[Comparison.LT_EQ, "Literal <= Comparison"],
		[Comparison.GT_EQ, "Literal >= Comparison"],
		[for_loop, """Builtin For Any Contextual Lazy 
	Parameter count Number
	Parameter value_name ValueName Quote None
	Parameter expression Any"""],
		[for_each_loop, """Builtin ForEach Any Contextual Lazy
	Parameter value_name ValueName
	Parameter values Any True
	Parameter expression Any"""],
		[while_loop, """Builtin While Any Contextual Lazy
	Parameter condition Boolean
	Parameter expression Any"""],
		[if_branch, """Builtin If Any Contextual Lazy
	Parameter condition Boolean
	Parameter consequent Any
	Parameter alternative Any Quote None"""],
		[logical_not, "Builtin Not Boolean Standalone Parameter expression Boolean"],
		[logical_all, "Builtin All Boolean Contextual Lazy Parameter expressions Boolean True"],
		[logical_any, "Builtin Some Boolean Contextual Lazy Parameter expressions Boolean True"],
		[sum, "Builtin Sum Number Unwrap Parameter operands Number True"],
		[compare, """Builtin Compare Boolean Unwrap
	Parameter left Number
	Parameter comparison Comparison
	Parameter right Number
		"""],
	],
)))
