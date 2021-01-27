#ifndef BRUSHLINK_ELEMENT_NAME_H
#define BRUSHLINK_ELEMENT_NAME_H

#include "NamedType.hpp"

namespace Command
{

struct ElementNameTag
{
	static Farb::HString GetName() { return "ElementName"; }
};
using ElementName = Farb::NamedType<Farb::HString, ElementNameTag>;

} // namespace Command

#endif // BRUSHLINK_ELEMENT_NAME_H
