
namespace Command
{
	
abstract class Root : EnumSet<ElementType>
{}

abstract class Kingdom : EnumSet<Phylum>
{}

abstract class Phylum : EnumSet
{}

enum ElementType
{
	Condition,
	Action,
	Selector,
	Target,
	UnitType,
	AbilityType,
	AttributeType,	
	Number,
	Termination // executes the command if the ending would otherwise be ambiguous
}

enum Condition
{
	SelectorIsNotEmpty,
	SelectorIncludesSelf, // for example when above or below threshold
	TimeHasPassed
}

enum Action
{
	Move = Enum.GetValues(typeof(Condition)).Last() + 1
	Attack,
	Cast,
	Follow, // can be used to move along a line or follow a unit
	Repeat,
	Stop,
	ClearConditionalCommands,
}

enum Selector
{
	
}

struct Element
{
	ElementType type;
	
	private object data;
	
	public int I => (int)this.data;
	public bool B => (bool)this.data;
	
	public Type Type
	{
		get
		{
			return this.data?.GetType();
		}
	}
}

enum ElementType
{
	// Condition
	SelectorIsNotEmpty,
	SelectorIncludesSelf, // for example when above or below threshold
	TimeHasPassed, //
	
	// Action
	Move,
	Attack,
	Cast,
	Follow, // can be used to move along a line or follow a unit
	Repeat,
	Stop,
	ClearConditionalCommands,
	
	// Selector
	CurrentSelection,
	Area,
	Square,
	OnScreen,
	OfUnitType,
	WithAbilityType,
	Number,
	Allies,
	Enemies,
	WithinRange, // followed by a number means + N
	Closest,
	
	BelowAttributeThreshold, // followed by an attribute and number in any order
	AboveAttributeThreshold,
	WithMaxAttribute,
	WithMinAttribute,
	
	// Target
	Point,
	Direction,
	Line,
	Area,
	Square,
	Selection,
	Single,
	GroupSizeRatio, // default 1, can be specified, relative to selection
	
	// UnitType
	Worker,
	Transport,
	Scout,
	
	// AbilityType
	Healing,
	Movement,
	Damage,
	CrowdControl,
	Building,
	
	// AttributeType
	Health,
	DPS,
	Energy,
	
	// Number
	Zero,
	One,
	Two,
	Three,
	Four,
	Five,
	Six,
	Seven,
	Eight,
	Nine,
	
	// Termination
	Termination
}

enum ElementCategory
{
	Condition,
	Action,
	Selector,
	Target,
	UnitType,
	AbilityType,
	AttributeType,	
	Number,
	Termination // executes the command if the ending would otherwise be ambiguous
}

ElementCategory GetCategory(ElementType type)
{
	if (type == ElementType.Termination)
		return ElementCategory.Termination;
	else if (type >= ElementType.Zero)
		return ElementCategory.Number;
	else if (type >= ElementType.Health)
		return ElementCategory.AttributeType;
	else if (type >= ElementType.Healing)
		return ElementCategory.AbilityType;
	else if (type >= ElementType.Worker)
		return ElementCategory.UnitType;
	else if (type >= ElementType.Point)
		return ElementCategory.Target;
	else if (type >= ElementType.CurrentSelection)
		return ElementCategory.Selector;
	else if (type >= ElementType.Move)
		return ElementCategory.Action;
	else if (type >= ElementType.SelectorIsNotEmpty)
		return ElementCategory.Condition;
}

static readonly Dictionary<ElementType, ElementCategory>() ElementTypeToCategory = 
	Enum.GetValues(typeof(ElementType)).Cast<ElementType>()
	.ToDictionary((i) => i, (i) => GetCategory(i));
Dictionary<ElementCategory, ElementType> ElementCategoryToType = ElementTypeToCategory.ToDictionary((i) => i.Value, (i) => i.Key);

class Element
{
	ElementType type;
	
}

// rmf todo: validation of the sequence of elements, probably a running stack of upcoming requirements
// how to resolve ambiguity of whether a selector modifies the previous selector or begins a new one?
// aka distance from x to y


class Fragment
{
	List<Element> elements;
}

}