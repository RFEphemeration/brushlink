#ifndef BRUSHLINK_CONTEXT_H
#define BRUSHLINK_CONTEXT_H

namespace Command
{

struct Context
{
	Set<ElementType::Enum> GetAllowedWithImplied(Set<ElementType::Enum> allowed) const;
};

} // namespace Command

#endif // BRUSHLINK_CONTEXT_H
