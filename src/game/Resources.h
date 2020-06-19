#pragma once
#ifndef BRUSHLINK_RESOURCES_H
#define BRUSHLINK_RESOURCES_H

#include "NamedType.hpp"

namespace Brushlink
{

using namespace Farb;

struct EnergyTag
{
	static HString GetName() { return "Energy"; }
};
using Energy = NamedType<int, EnergyTag>;


} // namespace Brushlink

#endif // BRUSHLINK_RESOURCES_H

