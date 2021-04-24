import sys

from burl.test import run_tests
from burl.run import run_module, run_repl, run_compose

if __name__ == "__main__":
	if len(sys.argv) < 2:
		print("This module requires arguments, either test, repl, compose, or run <module> <function> <args...>")
	elif sys.argv[1] == "test":
		run_tests()
	elif sys.argv[1] == "repl":
		run_repl()
	elif sys.argv[1] == "compose":
		run_compose()
	elif sys.argv[1] == "run":
		if len(sys.argv) < 3:
			print("Run requires a burl module name")
		else:
			module = sys.argv[2]
			function = sys.argv[3] if len(sys.argv) > 3 else "Main"
			args = sys.argv[4:]
			run_module(module, function, args)
	else:
		print("Unrecognized argument %s, expecting either test or repl" % sys.argv[0])