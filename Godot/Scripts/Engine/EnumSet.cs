abstract class EnumSet
{}

abstract class EnumSet<TChild> : EnumSet where TChild : EnumSet
{
	protected IEnumerable<TChild> Children { get; private set; }
	
	public bool Contains(TChild child)
	{
		return Children.Contains(child);
	}
}
