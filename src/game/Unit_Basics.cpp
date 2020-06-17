
#include "Unit_Basics.h"

namespace Brushlink
{

Unit_Type GetRandomUnitType(std::mt19937 generator)
{
	// @Cleanup hardcoded values for min/max unit type
	std::uniform_int_distribution<> range{0, 2};
	return static_cast<Unit_Type>(range(generator));
};

} // namespace Brushlink
