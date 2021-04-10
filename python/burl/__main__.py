import sys

from burl.test import run_tests
from burl.repl import run_repl


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("This module requires arguments, either test or repl")
    elif sys.argv[1] == "test":
        run_tests()
    elif sys.argv[1] == "repl":
        run_repl()
    else:
        print("Unrecognized argument %s, expecting either test or repl" % sys.argv[0])