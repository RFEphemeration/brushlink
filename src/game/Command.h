

namespace Command
{

enum class ElementCategory
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

enum class ElementType
{
	// Condition
	SelectorIsNotEmpty,
	SelectorIncludesSelf, // for example when above or below threshold
	TimeHasPassed,
	
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
	Percent,
	Quartile,
	Absolute,
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

constexpr ElementCategory CategoryOf(ElementType type)
{
	switch(type)
	{
	// Condition
	case SelectorIsNotEmpty:
	case SelectorIncludesSelf: // for example when above or below threshold
	case TimeHasPassed:
		return ElementCategory::Condition;

	// Action
	case Move:
	case Attack:
	case Cast:
	case Follow: // can be used to move along a line or follow a unit
	case Repeat:
	case Stop:
	case ClearConditionalCommands:
		return ElementCategory::Action;
	
	// Selector
	case CurrentSelection:
	case Area:
	case Square:
	case OnScreen:
	case OfUnitType:
	case WithAbilityType:
	case Number:
	case Allies:
	case Enemies:
	case WithinRange: // followed by a number means + N
	case Closest:

	case BelowAttributeThreshold: // followed by an attribute and number in any order
	case AboveAttributeThreshold:
	case WithMaxAttribute:
	case WithMinAttribute:
		return ElementCategory::Selector;

	// Target
	case Point:
	case Direction:
	case Line:
	case Area:
	case Square:
	case Selection:
	case Single:
	case GroupSizeRatio: // default 1, can be specified, relative to selection 
		return ElementCategory::Target;

	// UnitType
	case Worker:
	case Transport:
	case Scout:
		return ElementCategory::UnitType;

	// AbilityType
	case Healing:
	case Movement:
	case Damage:
	case CrowdControl:
	case Building:
		return ElementCategory::AbilityType;
	
	// AttributeType
	case Health:
	case DPS:
	case Energy:
		return ElementCategory::AttributeType;
	
	// Number
	case Zero:
	case One:
	case Two:
	case Three:
	case Four:
	case Five:
	case Six:
	case Seven:
	case Eight:
	case Nine:
		return ElementCategory::Number

	// Termination
	case Termination:
		return ElementCategory::Termination
	}
}

} // namespace Command
