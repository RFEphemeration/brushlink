import sys

from burl.test import run_tests
from burl.repl import run_repl
from burl.run import run_module


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("This module requires arguments, either test, repl, or run <module> <function> <args...>")
    elif sys.argv[1] == "test":
        run_tests()
    elif sys.argv[1] == "repl":
        run_repl()
    elif sys.argv[1] == "run":
    	if len(sys.argv) < 4:
    		print("Run requires both a module name and a function name")
    	else:
    		run_module(sys.argv[2], sys.argv[3], sys.argv[4:])
    else:
        print("Unrecognized argument %s, expecting either test or repl" % sys.argv[0])