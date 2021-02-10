
#include "Context.h"
#include "Game.h"
#include "Player.h"

namespace Command
{

// should probably make this a per-player setting
const Table<Variant_Type, Set<Variant_Type>> allowed_with_implied{
	//{ET::Selector, {ET::Set, ET::Filter, ET::Group_Size, ET::Superlative}},
	//{ET::Location, {ET::Point, ET::Line, ET::Direction, ET::Area}},
	//{ET::Set, {ET::Number}} // Command Group, this feels like it would come up unintentionally too often
	{Variant_Type::Number, {Variant_Type::Digit}} // Number literal
};

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

Context Context::MakeChild(Scope new_scope)
{
	Context child;
	child.scope = new_scope;
	child.parent = this;
	return child;
}

ErrorOr<std::vector<Variant>> Context::GetNamedValue(ValueName name)
{
	if (Contains(values, name))
	{
		return values[name];
	}

	if (Contains(arguments, name))
	{
		return arguments[name];
	}

	if (parent != nullptr)
	{
		return parent->Get(name);
	}

	return Error("No value found with name " + name.value);
}

ErrorOr<Brushlink::Unit &> GetUnit(Brushlink::UnitID id)
{
	if (Contains(game->world.units, id))
	{
		return game->world.units[id];
	}
	else
	{
		return Error("InvalidID");
	}
}

ErrorOr<Success> Context::Recurse()
{
	if (scope == Scope::Function)
	{
		recurse = true;
	}
	else
	{
		if (parent)
		{
			return parent->Recurse();
		}
		return Error("Global scope context can't recurse");
	}
}

ErrorOr<Success> Context::SetArgument(ValueName name, std::vector<Variant> value)
{
	if (scope == Scope::Function
		|| scope == Scope::Global)
	{
		if (!Contains(arguments, name))
		{
			return Error("Invalid argument name: " + name.value);
		}
		arguments[name] = value;
		return Success{};
	}
	else
	{
		if (parent)
		{
			return parent->SetArgument(name, element);
		}
		return Error("Context parent is unexpectedly null");
	}
	
}

ErrorOr<std::vector<Variant>> Context::SetLocal(ValueName name, std::vector<Variant> value)
{
	if (scope == Scope::Global)
	{
		return Error("We are in a global context, cannot set local");
	}
	else if (scope == Scope::Function)
	{
		values[name] = value;
		return value;
	}
	else
	{
		if (parent)
		{
			return parent->SetLocal(name, element);
		}
		return Error("Context parent is unexpectedly null");
	}
}

ErrorOr<std::vector<Variant>> Context::SetGlobal(ValueName name, std::vector<Variant> value)
{
	if (scope == Scope::Global)
	{
		values[name] = value;
		return value;
	}
	else
	{
		if (parent)
		{
			return parent->SetGlobal(name, element);
		}
		return Error("Context parent is unexpectedly null");
	}
}

} // namespace Command
