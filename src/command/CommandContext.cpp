#include "CommandContext.h"

namespace Command
{

void CommandContext::InitElementDictionary()
{
	element_dictionary.clear();

	element_dictionary.insert({
		{"Select", MakeContextFunctionWithActors(
			ElementType.Action,
			CommandContext::Select,
			{{ParamSingleRequired(ElementType.Selector)}})}
		// maybe numbers shouldn't be literals because of left parameter for *10
		{"Zero", MakeLiteral(Number(0))},
		{"One", MakeLiteral(Number(1))}
	});

	// todo: load user defined words from save
}

ErrorOr<std::unique_ptr<CommandElement> > CommandContext::GetNewCommandElement(HString name)
{
	if (element_dictionary.contains(name))
	{
		return element_dictionary[name].DeepCopy();
	}
	else
	{
		return Error("Couldn't find element with name" + name.ToCString())
	}
}

} // namespace Command
