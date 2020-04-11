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
			{{ParamSingleRequired(ElementType.Selector)}}
		)},
		{"Move", MakeContextFunctionWithActors(
			ElementType.Action,
			CommandContext::Move,
			{
				{ParamSingleRequired(ElementType.Selector)},
				{ParamSingleRequired(ElementType.Location)}
			}
		)},
		{"Attack", MakeContextFunctionWithActors(
			ElementType.Action,
			CommandContext::Move,
			{
				{ParamSingleRequired(ElementType.Selector)},
				{ParamSingleRequired(ElementType.Selector)}
			}
		)},
		{"SetCommandGroup", MakeContextFunctionWithActors(
			ElementType.Action,
			CommandContext::SetCommandGroup,
			{
				{ParamSingleRequired(ElementType.Selector)}
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
			{ParamSingleRequired(ElementType.Number)}
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
