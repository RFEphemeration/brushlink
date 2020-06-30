#ifndef BRUSHLINK_CONTEXT_H
#define BRUSHLINK_CONTEXT_H

namespace Command
{

struct Context
{
	virtual ~Context() = default;
	Set<Variant_Type> GetAllowedWithImplied(Set<Variant_Type> allowed) const;
};

// multiple sub types
// root player context - append elements, global variables, get elements, tree navigation
// unit call context - setup some assumed variables
// function call context - parameters, tail recursion, etc
// scope inside function call context - for loops, etc
// parsing definitions context


} // namespace Command

#endif // BRUSHLINK_CONTEXT_H
