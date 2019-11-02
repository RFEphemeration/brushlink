#ifndef BRUSHLINK_ELEMENT_DEFINITIONS_H
#define BRUSHLINK_ELEMENT_DEFINITIONS_H

namespace Command
{

// should be able to specialize without ErrorOr if it isn't something that could fail


// Selectors
ErrorOr<UnitGroup> selector(
	const CommandContext & context,
	UnitGroup set,
	OptionalRepeatable<Selector_Filter> filters,
	Optional<Selector_GroupSize> group_size,
	Optional<Selector_Superlative> superlative);

UnitGroup selector_union(
	const CommandContext& context,
	const ElementNode & left,
	const ElementNode & right);

UnitGroup enemies(
	const CommandContext& context);

UnitGroup allies(
	const CommandContext& context);

UnitGroup current_selection(
	const CommandContext& context);

ErrorOr<UnitGroup> actors(
	const CommandContext& context);

ErrorOr<UnitGroup> command_group(
	const CommandContext& context
	const ElementNode & number);

Selector_Filter within_range(
	const CommandContext & context,
	Number distance_modifier);

Selector_Filter in_area(
	const CommandContext & context,
	Area area);

// todo: as_individuals, maybe cutting this...

ErrorOr<Selector_GroupSize> actor_ratio(
	const CommandContext & context,
	Number ratio);

ErrorOr<Selector_GroupSize> group_ratio(
	const CommandContext & context,
	Number ratio);

Selector_GroupSize group_size(
	const CommandContext & context,
	Number number);

Selector_Superlative closest(
	const CommandContext & context);

Selector_Superlative max_attribute(
	const CommandContext & context
	Attribute_Type attribute);

Selector_Superlative min_attribute(
	const CommandContext & context
	Attribute_Type attribute);


} // namespace Command

#endif // BRUSHLINK_ELEMENT_DEFINITIONS_H