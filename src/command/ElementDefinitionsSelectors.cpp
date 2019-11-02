

// should be able to specialize without ErrorOr if it isn't something that could fail
ErrorOr<UnitGroup> selector(
	const CommandContext & context,
	UnitGroup set,
	OptionalRepeatable<Selector_Filter> filters,
	// as_individuals just continues to cause problems, eh?
	// maybe it's a modifier on actions, not group_size
	// or a transformation that happens before evaluation
	// such as into for each unit in selector perform action
	Optional<Selector_GroupSize> group_size,
	Optional<Selector_Superlative> superlative)
{
	UnitGroup group = set;
	for (auto & filter : filters)
	{
		// todo: filter within range as_individuals
		group = filter(context, group);
	}

	if (group_size.has_value())
	{
		Number size = CHECK_RETURN(group_size.value()(context, group));
		// only apply group size if our group is bigger
		if (size < group.members.size())
		{
			if (superlative.has_value())
			{
				// todo: superlative closest as_individuals
				group = superlative.value()(context, size, group);
			}
			else
			{
				// do we just pick randomly here?
				// just cut off the end?
				// or should we try to reason about what the filters were?
				group.members.resize(size.value);
			}
		}
	}
	else if (superlative.has_value())
	{
		// superlative without group size implies group size 1
		group = superlative.value()(context, Number{1}, group);
	}
	
	return group;
}

UnitGroup selector_union(
	const CommandContext& context,
	const ElementNode & left,
	const ElementNode & right)
{
	std::unordered_set<UnitID> combined;

	auto leftGroup = CHECK_RETURN(left.EvaluateTo<UnitGroup>(context));
	auto rightGroup = CHECK_RETURN(right.EvaluateTo<UnitGroup>(context));

	for(auto id : leftGroup.members)
	{
		combined.insert(id);
	}

	for (auto id : rightGroup.members)
	{
		combined.insert(id);
	}

	return UnitGroup{combined};
}

UnitGroup enemies(
	const CommandContext& context)
{
	return context.GetAllEnemyUnits();
}

UnitGroup allies(
	const CommandContext& context)
{
	return context.GetAllAlliedUnits();
}

UnitGroup current_selection(
	const CommandContext& context)
{
	return context.GetCurrentSelectedUnits();
}

ErrorOr<UnitGroup> actors(
	const CommandContext& context)
{
	return context.GetActorsForInProgressCommand();
}

ErrorOr<UnitGroup> command_group(
	const CommandContext& context
	const ElementNode & number)
{
	Number result = CHECK_RETURN(number.EvaluateTo<Number>(context));
	return context.GetUnitsInCommandGroup(result);
}

Selector_Filter within_range(
	const CommandContext & context,
	Number distance_modifier)
{
	static auto evaluate = [](
		const CommandContext & context,
		Number distance_modifier,
		UnitGroup group)
	{
		UnitGroup actors = context.GetActorsForInProgressCommand();

		std::vector<CRef<Unit> > actorUnits;

		for (auto unitID : actors.members)
		{
			actorUnits.push_back(context.GetUnit(unitID));
		}

		UnitGroup new_group;

		for(auto unitID : group.members)
		{
			const Unit & target = context.GetUnit(unitID);

			for(const Unit & actor : actorUnits)
			{
				Number range = actor.GetRange() + distance_modifier;
				if (context.UnitDistanceLessThan(actor, target, range))
				{
					new_group.members.push_back(unitID);
				}
			}
		}

		return new_group;
	}

	return CurriedFunctor{
		FunctionPointer{evaluate},
		distance_modifier
	};
}

Selector_Filter in_area(
	const CommandContext & context,
	Area area)
{
	static auto evaluate = [](
		const CommandContext & context,
		Area area,
		UnitGroup group)
	{
		UnitGroup new_group;

		for(auto unitID : group.members)
		{
			const Unit & target = context.GetUnit(unitID);

			if (area.Contains(target.GetPosition()))
			{
				new_group.members.push_back(unitID);
			}
		}

		return new_group;
	}

	return CurriedFunctor{
		FunctionPointer{evaluate},
		area
	};
}

// Group Size
ErrorOr<Selector_GroupSize> actor_ratio(
	const CommandContext & context,
	Number ratio)
{
	if (ratio.value <= 0)
	{
		return Error("Invalid Ratio <= 0");
	}
	static auto evaluate = [](
		const CommandContext & context,
		Number ratio,
		UnitGroup group)
	{
		// if this is in the actors selection, should probably be equal to group ratio?
		// or should we just prevent anything that uses GetActors from being called
		// before Actors has been defined
		UnitGroup actors = CHECK_RETURN(context.GetActorsForInProgressCommand());

		Number group_size{ actors.members.size() / ratio.value };
		if (group_size.value == 0)
		{
			group_size.value = 1;
		}

		return group_size;
	};

	return CurriedFunctor{
		FunctionPointer{evaluate},
		ratio
	};
}

ErrorOr<Selector_GroupSize> group_ratio(
	const CommandContext & context,
	Number ratio)
{
	if (ratio.value <= 0)
	{
		return Error("Invalid Ratio <= 0");
	}

	static auto evaluate = [](
		Number ratio,
		const CommandContext & context,
		UnitGroup group)
	{
		Number group_size{ group.members.size() / ratio.value };
		if (group_size.value == 0)
		{
			group_size.value = 1;
		}

		return group_size;
	};

	return CurriedFunctor{
		FunctionPointer{evaluate},
		ratio
	};
}

Selector_GroupSize group_size(
	const CommandContext & context,
	Number number)
{
	static auto evaluate = [](
		Number number,
		const CommandContext & context,
		UnitGroup group)
	{
		return number;
	};

	return CurriedFunctor{
		FunctionPointer{evaluate},
		number
	};
}

Selector_Superlative superlative_helper(
	Functor<int, const CommandContext & context, const Unit &> get_metric,
	bool min_metric = true)
{
	static auto evaluate = [](
		Functor<int, const CommandContext & context, const Unit &> get_metric,
		bool min_metric,
		const CommandContext & context,
		Number group_size,
		UnitGroup group)
	{
		std::vector<std::pair<UnitID, int> > unitMetrics;

		for (auto unitID : group.members)
		{
			unitMetrics.push_back({unitID, get_metric(unitID)});
		}

		auto comparator = min_metric
			? [](const auto & a, const auto & b)
				{
					return a.second < b.second;
				}
			: [](const auto & a, const auto & b)
				{
					return a.second > b.second;
				};
		std::nth_element(
			unitMetrics.begin(),
			unitMetrics.begin() + group_size,
			unitMetrics.end(),
			comparator
		);

		UnitGroup new_group;
		for (int i = 0; i < group_size; i++)
		{
			new_group.members.push_back(unitMetrics[i].first);
		}
		return new_group;
	}

	return CurriedFunctor{
		CurriedFunctor{
			FunctionPointer{evaluate},
			get_metric
		},
		min_metric
	};
}

Selector_Superlative closest(
	const CommandContext & context)
{
	static auto getDistanceToNearestActor = [](
		std::vector<CRef<Unit> > actorUnits,
		const CommandContext & context,
		UnitID unitID)
	{
		const Unit & target = context.GetUnit(unitID);
		int minDistance = kMaxInt;
		for (const Unit & actor : actorUnits )
		{
			int distance = context.GetDistanceBetween(actor, target);
			if (distance < minDistance)
			{
				minDistance = distance;
			}
		}
		return minDistance
	}

	std::vector<CRef<Unit> > actorUnits;
	UnitGroup actors = context.GetActorsForInProgressCommand();
	for (auto unitID : actors.members)
	{
		actorUnits.push_back(context.GetUnit(unitID));
	}

	return superlative_helper(
		CurriedFunctor{
			FunctionPointer{getDistanceToNearestActor},
			actorUnits
		}
	);
}

Selector_Superlative max_attribute(
	const CommandContext & context
	Attribute_Type attribute)
{
	static auto getAttributeValue = [](
		Attribute_Type attribute,
		const CommandContext & context,
		UnitID unitID)
	{
		const Unit & target = context.GetUnit(unitID);
		return target.GetAttribute(attribute);
	}

	return superlative_helper(
		CurriedFunctor{
			FunctionPointer{getAttributeValue},
			attribute
		},
		false
	);
}

Selector_Superlative min_attribute(
	const CommandContext & context
	Attribute_Type attribute)
{
	static auto getAttributeValue = [](
		Attribute_Type attribute,
		const CommandContext & context,
		UnitID unitID)
	{
		const Unit & target = context.GetUnit(unitID);
		return target.GetAttribute(attribute);
	}

	return superlative_helper(
		CurriedFunctor{
			FunctionPointer{getAttributeValue},
			attribute
		}
	);
}

