#ifndef BRUSHLINK_CONTEXT_H
#define BRUSHLINK_CONTEXT_H

namespace Command
{

struct Context
{
	virtual ~Context() = default;
	Set<ElementType::Enum> GetAllowedWithImplied(Set<ElementType::Enum> allowed) const;
};

// multiple sub types
// root player context - append elements, global variables, get elements, tree navigation
// unit call context - setup some assumed variables
// function call context - parameters, tail recursion, etc
// scope inside function call context - for loops, etc


} // namespace Command

#endif // BRUSHLINK_CONTEXT_H
