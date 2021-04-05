from command import *


def main():
	repl = Context(root)
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
					ast = ParseNode.parse("\n".join(lines));
					value = ast.evaluate(repl)
					name = Value("$" + str(index), "ValueName")
					print(name.value + " = " + value.__str__())
					index += 1
					repl.set(name, value)
				except EvaluationError as e:
					print("EvaluationError: " + e.__str__())
				lines = []

if __name__ == "__main__":
	main()