#pragma once
#ifndef BRUSHLINK_RESOURCES_H
#define BRUSHLINK_RESOURCES_H

namespace Brushlink
{

struct EnergyTag
{
	static HString GetName() { return "Energy"; }
};
using Energy = NamedType<uint, Energy>;


} // namespace Brushlink

#endif // BRUSHLINK_RESOURCES_H

