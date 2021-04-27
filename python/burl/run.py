from burl.language import Context, EvaluationError, Value
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


def run_repl():
	repl = Context()
	first_prompt = ": "
	second_prompt = "| "
	lines = []
	index = 0
	while True:
		if not lines:
			prompt = str(index)
			prompt = "$" + prompt
			line = input(prompt + first_prompt)
			if line:
				if line == "Exit":
					return
				else:
					lines.append(line)
		else:
			prompt = " " * len(str(index)) + " ";
			line = input(prompt + second_prompt)
			if line:
				lines.append(line)
			else:
				print ("\033[A                             \033[A")
				try:
					ast = ParseNode.parse("\n".join(lines))
					value = ast.evaluate(repl)
					name = Value("$" + str(index), "ValueName")
					print(name.value + " = " + value.__str__())
					index += 1
					repl.set(name, value)
				except EvaluationError as e:
					print("EvaluationError: " + e.__str__())
				lines = []


def run_compose():
	# todo: can we split cursor and active_tab into their own context separate from the cursor evaluation?
	repl = Context()
	ParseNode.parse("LoadModule composition").evaluate(repl)
	prompt = ": "
	cursor = None
	active_tab = None
	tabs = frozenset()
	while True:
		if cursor is None:
			cursor = repl.parse_eval("Set cursor Cursor.Make Quote Sequence")
			active_tab = Value("Any", "Type")
			repl.set(Value("active_tab", "ValueName"), active_tab)
			tabs = repl.parse_eval("AllTypes")
			tabs = [t.value for t in tabs.value]

		options = repl.parse_eval("Cursor.GetAllowedArgumentTypes Get cursor")
		if not options.value: # None or empty frozenset
			print("No open parameter at cursor");
			elements = []
			value_names = []
		else:
			print("Allowed Types: " + " ".join([o.value for o in options.value]))
			if active_tab != "Any" and options.value is not None and active_tab not in options.value and Value('Any', 'Type') not in options.value:
				active_tab = next(iter(options.value))
				repl.set(Value("active_tab", "ValueName"), active_tab)

			elements = repl.parse_eval("DefinitionsOfType Get active_tab")
			elements = [e.value for e in elements.value]
			value_names = repl.parse_eval("ValuesOfType Get active_tab")
			value_names = [n.value for n in value_names.value]

		print("### Command ###")
		print(cursor.value.node)

		print("### Tabs ###")
		print(" ".join(tabs))

		print("### Meta ###")
		print("Exit Evaluate Skip Tab <type>")

		if options.value:
			print("### Elements (%s) ###" % active_tab.value)
			print(" ".join(elements))

			print("### Values (%s) ###" % active_tab.value)
			print(" ".join(value_names))

		try:
			line = input(prompt)
			if line == "Exit":
				break
			if line == "Evaluate":
				value = repl.parse_eval("Evaluate Cursor.GetEvalNode Get cursor")
				print(value)
			elif line == "Skip":
				success = cursor.value.next_node()
				if not success:
					print("Cursor could not skip")

			elif line.startswith("Tab "):
				tab = Value(line.split()[-1], "Type")
				if options.value is None or (
					tab not in options.value and Value("Any", "Type") not in options.value):
					print("Cursor does not allow values of type %s" % tab.value)
				else:
					active_tab = tab
					repl.set(Value("active_tab", "ValueName"), active_tab)

			elif line in elements:
				repl.parse_eval("Cursor.InsertArgument Get cursor Quote " + line)
			elif line in value_names:
				repl.parse_eval("Cursor.InsertArgument Get cursor Quote Get " + line)
			elif active_tab.value == "ValueName":
				repl.parse_eval("Cursor.InsertArgument Get cursor Quote " + line)
			elif active_tab.value == "Type":
				if repl.is_known_type(line):
					repl.parse_eval("Cursor.InsertArgument Get cursor Quote " + line)
				else:
					print("Uknown Type " + line)
			elif active_tab.value == "Number":
				repl.parse_eval("Cursor.InsertArgument Get cursor Quote " + line)
		except EvaluationError as e:
			print("EvaluationError: " + e.__str__())
