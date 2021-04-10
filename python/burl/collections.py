from burl.language import Module, ModuleDictionary
from burl.builtin import builtin_tools, make_context

ModuleDictionary.instance().add_module(Module('collections', context=make_context(
	temp_parent=builtin_tools,
	types={
		'HashSet',
	},
	evaluations=[
		[frozenset(), "Literal HashSet.Empty HashSet"],
		[frozenset, "Builtin HashSet.Make HashSet Parameter values Any True"],
		[len, "Builtin HashSet.Count Number Unwrap Parameter set HashSet"],
		[frozenset.union, """Builtin HashSet.Union HashSet Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
		[frozenset.intersection, """Builtin HashSet.Intersect HashSet Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
		[frozenset.difference, """Builtin HashSet.Difference HashSet Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
		[frozenset.symmetric_difference, """Builtin HashSet.SymDifference HashSet Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
		[frozenset.isdisjoint, """Builtin HashSet.Disjoint Boolean Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
		[frozenset.issubset, """Builtin HashSet.Subset Boolean Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
		[frozenset.issuperset, """Builtin HashSet.Superset Boolean Unwrap
	Parameter a HashSet
	Parameter b HashSet"""],
	],
)))