from burl.language import EvalNode, Context, EvaluationError, Literal, Element, Value


class UnboundSymbol(Element):
	def __init__(self, name):
		# consider passing a repeatable not required Any parameter here
		# would probably be a bad idea, this likely means declarations are in order
		Element.__init__(self, name, 'Any')
		self.binding = None

	def evaluate(self, context, unevaluated_arguments):
		if not self.binding:
			symbol_type = context.get_symbol_type(self.name)

			if symbol_type == 'Element':
				self.binding = context.get_definition(self.name)
				self.element_type = self.binding.element_type
			else:
				# ValueName or Type
				# this is inefficient when might be passed in an element defined literal anyway
				self.binding = Literal(self.name, symbol_type, self.name)
				self.element_type = symbol_type

		return self.binding.evaluate(context, unevaluated_arguments)


class Skip():
	def __init__(self):
		pass


# Parsing
class ParseNode:
	def __init__(self, contents, indentation, end_children = None, children = None):
		self.contents = contents
		self.indentation = indentation
		self.end_children = end_children if end_children is not None else []
		self.children = children if children is not None else []

	def __str__(self, indentation=0):
		out = ("   " * indentation) + self.contents
		for child in self.end_children:
			out += "\n" + child.__str__(indentation + 2)
		for child in self.children:
			out += "\n" + child.__str__(indentation + 1)
		return out

	def __repr__(self):
		return self.__str__()

	@staticmethod
	def parse(code):
		nodes = []
		lines = code.splitlines()
		for line in lines:
			contents = line.lstrip('\t')
			indentation = len(line) - len(contents)
			nodes.append(ParseNode(contents, indentation))

		root_node = ParseNode('RootSequence', -1)
		for node in nodes:
			if node.contents == "":
				continue
			current = root_node
			while True:
				if current.children and node.indentation > (current.indentation + 1):
					current = current.children[-1]
				elif current.end_children and node.indentation > (current.indentation + 2):
					current = current.end_children[-1]
				else:
					break
			if node.indentation > current.indentation + 1:
				current.end_children.append(node)
			else:
				current.children.append(node)

		if len(root_node.children) == 1:
			return root_node.children[0]
		else:
			return root_node

	def evaluate(self, context):
		if self.contents == "RootSequence":
			if self.end_children:
				raise EvaluationError("RootSequence has end_children, since it's implicit it should never have things on the same line.")
			# this being hardcoded is probably not ideal? but fine for now
			evaluated_children = []
			for child in self.children:
				(child_evaluator, skips) = child.to_eval_node(context)
				if skips > 0:
					raise EvaluationError("Parser encountered Skips before any elements")
				evaluated_children.append(child_evaluator.evaluate(context))
			return evaluated_children[-1]
		else:
			(evaluator, skips) = self.to_eval_node(context)
			if skips > 0:
				raise EvaluationError("Parser encountered Skips before any elements")
			return evaluator.evaluate(context)

	def to_eval_node(self, context):
		root_node = None
		pre_skip_count = 0
		skip_count = 0
		for name in str.split(self.contents):
			if name.startswith("#"):
				break
			if not name:
				continue
			node = ParseNode.try_bind(context, name)
			if not root_node:
				if isinstance(node.element, Skip):
					pre_skip_count += 1
					continue
				root_node = node
			else:
				if isinstance(node.element, Skip):
					skip_count += 1
					continue
				else:
					skip_count = 0
				(success, skips_remaining) = root_node.append_argument(node, skip_count)
				if not success:
					if isinstance(root_node.element, UnboundSymbol):
						raise EvaluationError("Failed to append argument %s to unbound symbol %s. Are you missing a module load?" % (name, root_node.element.name))
					else:
						raise EvaluationError("Failed to append argument %s to %s" % (name, root_node.element.name))
		if root_node:
			last_node = root_node.get_last_argument()
		else:
			last_node = None
		if last_node == root_node and self.end_children:
			raise EvaluationError("Element %s has end children but only one element (or none) on the line" % root_node.element.name)

		child_skips = 0
		for child in self.end_children:
			(child_node, skips) = child.to_eval_node(context)
			child_skips += skips
			if not child_node:
				continue
			(success, child_skips) = last_node.try_append_to_mapped_arguments(child_node, child_skips)
			if not success:
				raise EvaluationError("incorrect argument %s for %s" % (child_node.element.name, last_node.element.name))
			child_skips = 0

		child_skips = 0
		for child in self.children:
			(child_node, skips) = child.to_eval_node(context)
			child_skips += skips
			if not child_node:
				continue
			(success, child_skips) = root_node.try_append_to_mapped_arguments(child_node, child_skips)
			if not success:
				raise EvaluationError("incorrect argument %s for %s" % (child_node.element.name, last_node.element.name))
			child_skips = 0 # reset after successful append
		return (root_node, pre_skip_count)

	@staticmethod
	def try_bind(context, name):
		if name.isdigit():
			element = Literal(name, 'Number', int(name))
		elif name[0] == "-" and name[1:].isdigit():
			element = Literal(name, 'Number', -1 * int(name[1:]))
		elif name == "Skip":
			return EvalNode(Skip())
		else:
			try:
				element = context.get_definition(name)
			except EvaluationError:
				if context.is_known_type(name):
					element = Literal(name, 'Type', name)
				else:
					element = UnboundSymbol(name)
		return EvalNode(element)

def context_parse_eval(self, code):
	return ParseNode.parse(code).evaluate(self)

Context.parse_eval = context_parse_eval
