



struct Search
{
	// Inputs -> Outputs -> Node Options
	Map<Set<TypeInfo>, Map<Set<TypeInfo>, Set<NodeName>>>> mappings;

	List<NodeName> FindPath(Set<TypeInfo> start, Set<TypeInfo> end);
}