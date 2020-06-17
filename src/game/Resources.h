#pragma once
#ifndef BRUSHLINK_RESOURCES_H
#define BRUSHLINK_RESOURCES_H

#include "NamedType.hpp"

namespace Brushlink
{

struct EnergyTag
{
	static HString GetName() { return "Energy"; }
};
using Energy = NamedType<uint, Energy>;


} // namespace Brushlink

#endif // BRUSHLINK_RESOURCES_H

