
#include "Context.h"

namespace Command
{

Set<Variant_Type> Context::GetAllowedWithImplied(Set<Variant_Type> allowed) const
{
	static Table<Set<Variant_Type>, Set<Variant_Type> > cache;
	if (Contains(cache, allowed))
	{
		return cache[allowed];
	}
	Set<Variant_Type> merged_type_set_with_implied;
	// allowed_with_implied might vary by player
	for (auto&& [allowed_type, type_set_with_implied] : allowed_with_implied)
	{
		if (!Contains(allowed, allowed_type))
		{
			continue;
		}
		merged_type_set_with_implied.merge(Set<ElementType::Enum>{type_set_with_implied});
	}
	for (auto&& type : merged_type_set_with_implied)
	{
		// don't double add something that is allowed as implied and not implied
		// @Cleanup do we need this?
		if (Contains(allowed, type))
		{
			// erase only invalidates the current iterator
			merged_type_set_with_implied.erase(type);
		}
	}
	cache[allowed] = merged_type_set_with_implied;
	return merged_type_set_with_implied;
}

} // namespace Command
