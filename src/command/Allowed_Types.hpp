#ifndef BRUSHLINK_ALLOWED_TYPES_HPP
#define BRUSHLINK_ALLOWED_TYPES_HPP

#include "Variant.h"

#include "CardTypes.h"

namespace Command
{

struct AllowedType
{
	// could consider including a location
	Variant_Type type;
	bool is_left;

	AllowedType(Variant_Type type, bool is_left = false)
		: type(type)
		, is_left(is_left)
	{ }
};

struct AllowedTypes
{
	// instructions don't show up in priority and are only in total_instruction
	std::vector<AllowedType> priority;

	Table<Variant_Type, int> total_right;
	Table<Variant_Type, int> total_left;
	Table<Instruction, int> total_instruction;

	inline void Append(AllowedType type)
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
};


} // namespace Command

#endif // BRUSHLINK_ALLOWED_TYPES_HPP
