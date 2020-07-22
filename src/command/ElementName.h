#ifndef BRUSHLINK_ELEMENT_NAME_H
#define BRUSHLINK_ELEMENT_NAME_H

#include "NamedType.hpp"

namespace Command
{

struct ElementNameTag
{
	static HString GetName() { return "ElementName"; }
};
using ElementName = NamedType<HString, ElementNameTag>;

} // namespace Command

#endif // BRUSHLINK_ELEMENT_NAME_H
