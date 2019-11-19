#include "Interpreter.h"

#include "ElementDictionary.h"

namespace Command
{

ErrorOr<Value> Interpreter::Evaluate(
	const CommandContext & context
	const ElementNode & node)
{
	auto decl = ElementDictionary::GetDeclaration(node.token.name);
	if (decl == nullptr)
	{
		return Error{"Couldn't find declaration for element " + node.token.name};
	}
	return decl->definition->Evaluate(context, node);
}

} // namespace Command
