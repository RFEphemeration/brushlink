#ifndef BRUSHLINK_INSTRUCTION_H
#define BRUSHLINK_INSTRUCTION_H

namespace Command
{

enum class Instruction_Type
{
	Evaluate,
	Cancel,
	Skip,
	Undo,
	Redo,
};

} // namespace Command

#endif // BRUSHLINK_INSTRUCTION_H
