#ifndef BRUSHLINK_CONTEXT_H
#define BRUSHLINK_CONTEXT_H

#include "BuiltinTypedefs.h"

#include "Basic_Types.h"
#include "Variant.h"
#include "Unit.h"

namespace Brushlink
{

struct Player;
struct Game;

} // namespace Brushlink


namespace Command
{

enum class Scope
{
	Global,
	Function,
	Element
};

struct Context
{
	Brushlink::Game * game {nullptr};
	Brushlink::Player * player {nullptr};
	Context * parent {nullptr};

	Scope scope;

	// these two are only used for function scope contexts
	// consider subclassing?
	bool recurse {false};
	Farb::Table<Brushlink::ValueName, std::vector<Variant> > values;

	Farb::Table<Brushlink::ValueName, std::vector<Variant> > arguments;

	virtual ~Context() = default;

	// internal functions
	Set<Variant_Type> GetAllowedWithImplied(Set<Variant_Type> allowed) const;
	Context MakeChild(Scope new_scope);
	ErrorOr<std::vector<Variant>> GetNamedValue(Brushlink::ValueName name);
	ErrorOr<Ref<Brushlink::Unit>> GetUnit(Brushlink::UnitID id);

	// exposed functions
	ErrorOr<Success> Recurse();
	ErrorOr<Success> SetArgument(ValueName name, std::vector<Variant> value);
	ErrorOr<Success> SetLocal(ValueName name, std::vector<Variant> value);
	ErrorOr<Success> SetGlobal(ValueName name, std::vector<Variant> value);
	ErrorOr<Variant> GetLast(ValueName name);
	ErrorOr<Variant> GetNth(ValueName name);
	ErrorOr<Number> Count(ValueName name);
	ErrorOr<Point> GetAveragePoint(Unit_Group group);
};

/*
struct PlayerContext : public Context
{
	Player * player;
	Dictionary hidden_elements;
	Dictionary exposed_elements;

	Table<ValueName, std::vector<Variant> > global_values;
};

struct UnitContext : public Context
{
};

struct FunctionContext : public Context
{
	Context * parent {nullptr};
	bool recurse;
	Table<ValueName, std::vector<Variant> > local_values;
};

struct ElementContext : public Context
{
	Context * parent {nullptr};
	Table<ValueName, std::vector<Variant> > arguments;
};
*/


// multiple sub types
// root player context - append elements, global variables, get elements, tree navigation
// unit call context - setup some assumed variables
// function call context - parameters, tail recursion, etc
// scope inside function call context - for loops, etc
// parsing definitions context


} // namespace Command

#endif // BRUSHLINK_CONTEXT_H
