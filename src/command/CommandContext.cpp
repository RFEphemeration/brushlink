#include "CommandContext.h"

namespace Command
{

using ElementType;

void CommandContext::InitElementDictionary()
{
	element_dictionary.clear();

	element_dictionary.insert({
		{"Select", MakeContextAction(
			Action,
			CommandContext::Select,
			{
				Param(Selector)
			}
		)},
		{"Move", MakeContextAction(
			Action,
			CommandContext::Move,
			{
				Param(Selector),
				Param(Location)
			}
		)},
		{"Attack", MakeContextAction(
			ElementType.Action,
			CommandContext::Move,
			{
				Param(Selector),
				Param(Selector)
			}
		)},
		{"SetCommandGroup", MakeContextAction(
			ElementType.Action,
			CommandContext::SetCommandGroup,
			{
				Param(Selector)
			}
		)},
		{"Enemies", MakeContextFunction(
			ElementType.Set,
			CommandContext::Enemies,
			{}
		)},
		{"Allies", MakeContextFunction(
			ElementType.Set,
			CommandContext::Allies,
			{}
		)},
		{"CurrentSelection", MakeContextFunction(
			ElementType.Set,
			CommandContext::CurrentSelection,
			{}
		)},
		{"Actors", MakeContextFunction(
			ElementType.Set,
			CommandContext::Actors,
			{}
		)},
		{"CommandGroup", MakeContextFunction(
			ElementType.Set,
			CommandContext::CommandGroup,
			{
				Param(Number)
			}
		)},
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
