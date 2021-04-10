from burl.language import Context, Module, ModuleDictionary, EvaluationError, Value
from burl.builtin import builtin_tools, make_context


def all_types(current_context):
	types = set()

	def merge_types(context):
		nonlocal types
		for t in context.types:
			types.add(Value(t, 'Type'))
		return False # navigate to full depth
	current_context.navigate_parents_and_modules(merge_types)
	return frozenset(types)


def definitions_of_type(current_context, element_type):
	definitions = set()

	def merge_definitions(context):
		nonlocal definitions
		# what to do about name conflicts?
		for name in context.definitions:
			# do we want elements that a parameter of type would accept?
			if context.definitions[name].element_type == element_type.value:
				# what is the type here? symbol name?
				definitions.add(Value(name, 'ValueName'))
		return False # navigate to full depth
	current_context.navigate_parents_and_modules(merge_definitions)
	return frozenset(definitions)


def values_of_type(current_context, element_type):
	values = {}

	def merge_values(context):
		nonlocal values
		# what to do about name conflicts?
		for name in context.values:
			# do we want elements that a parameter of type would accept?
			if context.values[name].element_type == element_type.value:
				values.add(Value(name, 'ValueName'))
		return False # navigate to full depth
	current_context.navigate_parents_and_modules(merge_values)
	return frozenset(values)


def evaluate(context, node):
	if node.element_type == 'EvaluationError':
		return node
	try:
		value = node.value.evaluate(context)
		if isinstance(value, Value):
			return value
		else:
			return Value(value, node.value.element.element_type)
	except EvaluationError as e:
		return Value(e, 'EvaluationError')


def append_argument(tree, arg):
	# this modifies the argument, which is not ideal
	try:
		success = tree.value.append_argument(arg.value)
		if success:
			return Value(tree.value, 'EvalNode')
		else:
			return Value('Could not append argument %s to %s' % (
				arg.value.element.name, tree.value.element.name), 'EvaluationError')
	except EvaluationError as e:
		return Value(e, 'EvaluationError')


ModuleDictionary.instance().add_module(Module('composition', context=make_context(
	temp_parent=builtin_tools,
	types={
		'EvaluationError',
	},
	evaluations=[
		[None, "LoadModule collections"],
		[all_types, "Builtin AllTypes HashSet Contextual"],
		[definitions_of_type, "Builtin DefinitionsOfType HashSet Contextual Parameter type Type"],
		[values_of_type, "Builtin ValuesOfType HashSet Contextual Parameter type Type"],
		[evaluate, "Builtin Evaluate Any Contextual Parameter node EvalNode"],
		[append_argument, """Builtin AppendArgument EvalNode Standalone
	Parameter tree EvalNode
	Parameter argument EvalNode"""],
	],
)))
