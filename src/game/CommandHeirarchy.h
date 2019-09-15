namespace Command
{

struct Command
{

}

struct Element
{

}

struct Selector : Element
{
	virtual List<Unit> Resolve(CommandContext context);
}

struct Intersection : Selector
{
	Selector previous;
	Selector next;
}

struct Union : Selector
{
	Selector previous;
	Selector next;
}

struct Action : Element
{
	Target target;
}

struct Move : Action
{
	Location to;
}

struct Target : Element
{

}

struct Location : Element
{

}

struct GroundPoint : Location
{

}

struct UnitPosition : Location
{
	Unit target;
}


}