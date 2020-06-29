#include "Allowed_Types.hpp"


void AllowedTypes::Append(AllowedType type)
{
	priority.push_back(type);
	if (type.is_left)
	{
		total_left[type.type]++;
	}
	else
	{
		total_right[type.type]++;
	}
}
