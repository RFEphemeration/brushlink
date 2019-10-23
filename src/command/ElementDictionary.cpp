#include "ElementDictionary.h"
#include "Parser.h"

namespace Command
{

std::pair<ElementName, ElementDeclaration>Decl(const ElementDeclaration& decl)
{
	return {decl.name, decl};
}

using namespace ElementType;

using Left = LeftParameter;

using Right = std::vector<ElementParameter>;

const std::map<ElementName, ElementDeclaration> ElementDictionary::declarations
{
	// rmf todo: how to state that this left parameter needs to be a root node?
	// Command
	Decl({ "Command", Command,
		Right{
			// todo: default with implied
			// what if some selectors aren't valid here?
			// is it just that they're not valid yet?
			// i.e. current_actor not valid until Command Arg 0 is complete?
			{ Selector, "Current_Selection" },
			{ Action, "Set_Current_Selection" },
			// how to require at least one of selector or action?
			// here it doesn't particularly matter
			// since setting the current selection to the current selection
			// is a no-op
			{ Termination }
		}
	}),

	// Action
	// it feels like there are two groups of actions
	// things done by units and things done by the player
	Decl({ "Set_Current_Selection", Action,
		Left{{ Selector, "Current_Selection" }},
	}),
	Decl({ "Assign_Command_Group", Action,
		Left{{ Selector, "Current_Selection" }},
		Right{{ Number }}
	}),
	Decl({ "Add_To_Command_Group", Action,
		Left{{ Selector, "Current_Selection" }},
		Right{{ Number }}
	}),
	Decl({ "Move", Action,
		Left{{ Selector, "Current_Selection" }},
		// could be changed to one_of Point, Line, Area
		Right{{ Location }}
	}),
	Decl({ "Follow", Action,
		Left{{ Selector, "Current_Selection" }},
		// one_of Line, Selector
		Right{{ Line }}
	}),
	// rmf todo: resolve Attack, Attack_Move, Fire_At
	Decl({ "Attack", Action,
		Left{{ Selector, "Current_Selection" }},
		Right{
			// rmf todo: one_of Selector, Location
			// or maybe these should be separate?
			// todo: default tree and merging
			{ Selector }
		}
	}),
	Decl({ "Fire_At", Action,
		Left{{ Selector, "Current_Selection" }}, 
		Right{
			{ Location }
		}
	}),
	Decl({ "Cast", Action,
		Left{{ Selector, "Current_Selection" }},
		Right{
			{ Ability_Type },
			{ Location }
		}
	}),

	// Selector
	Decl({ "Selector", Selector,
		Right{
			{ Set, Optional | Permutable },
			{ Group_Size, Optional | Permutable },
			{ Filter, Optional | Permutable | Repeatable },
			{ Superlative, Optional | Permutable }
		}
	}),
	Decl({ "Union", Selector,
		Left{{ Selector }},
		Right{{ Selector }}
	}),

	// Set
	Decl({ "Enemies", Set }),
	Decl({ "Allies", Set }),
	Decl({ "Current_Selection", Set }),
	Decl({ "Actors", Set }),
	Decl({ "Command_Group", Set, Right{{ Number }} }),

	// Filter
	Decl({ "Within_Range", Filter,
		Right{{ Number, "Zero" }}
	}),
	Decl({ "In_Area", Filter,
		Right{{ Area }} }),
	// Attribute_Threshold_Quintile right params Threshold_Type(Absolute,Relative), Attribute, Number

	// Group Size
	Decl({ "As_Individuals", Group_Size }),
	Decl({ "Actor_Ratio", Group_Size,
		Right{{ Number, "One" }}
	}),
	Decl({ "Group_Ratio", Group_Size,
		Right{{ Number, "One" }}
	}),
	Decl({ "Group_Size", Group_Size,
		Right{{ Number, "One" }}
	}),

	// Superlative
	Decl({ "Closest", Superlative }),
	Decl({ "Max_Attribute", Superlative,
		Right{{ Attribute_Type }}
	}),
	Decl({ "Min_Attribute", Superlative,
		Right{{ Attribute_Type }}
	}),

	// Location
	Decl({ "Location", Location,
		Right{
			// todo: change to one_of
			{ Point, Optional | Permutable },
			{ Line, Optional | Permutable },
			{ Area, Optional | Permutable }
		}
	}),

	// Point
	Decl({ "Coordinates", Point, Right{{ Number }, { Number }} }), // x, y
	Decl({ "Average_Position", Point, Right{{ Selector }} }),
	Decl({ "Center_Of", Point, Right{{ Area }} }),

	// Line
	Decl({ "Point_List", Line, Right{{ Point, Repeatable }} }),
	// not quite sure how to do mouse input yet
	Decl({ "Drawn_Line", Line, Right{{ Mouse_Input }} }),

	// Area
	Decl({ "Group_Area", Area, Right{{ Selector }} }),
	Decl({ "Drawn_Area", Area, Right{{ Mouse_Input }} }),
	// rmf note: it'd be nice if box_between and drawn_box were
	// just one element - Box. todo: one_of Mouse_Input, (Point Point)
	Decl({ "Drawn_Box", Area, Right{{ Mouse_Input }} }),
	Decl({ "Box_Between", Area,
		Right{
			{ Point }, // two corners
			{ Point }
		}
	}),
	Decl({ "Circle", Area,
		Right{
			{ Point }, // center
			{ Number } // radius
		}
	}),

	// Unit_Type
	Decl({ "Builder", Unit_Type }),
	Decl({ "Scout", Unit_Type }),
	Decl({ "Melee", Unit_Type }),
	Decl({ "Ranged", Unit_Type }),
	Decl({ "Tank", Unit_Type }),
	Decl({ "Healer", Unit_Type }),
	Decl({ "Support", Unit_Type }),

	// Attribute_Type
	Decl({ "Health", Attribute_Type }),
	Decl({ "Move_Speed", Attribute_Type }),
	Decl({ "Range", Attribute_Type }),

	// Ability_Type
	Decl({ "Movement", Ability_Type }),
	Decl({ "Heal", Ability_Type }),
	Decl({ "Area_Of_Affect", Ability_Type }),
	Decl({ "Build", Ability_Type }),
	Decl({ "Spawn", Ability_Type }),
	Decl({ "Buff", Ability_Type }),
	Decl({ "Debuff", Ability_Type }),

	// Number
	// tood: should probably add flag of Literal as requirement for left parameter
	Decl({ "Zero",  Number, Left{{ Number, Optional }} }),
	Decl({ "One",   Number, Left{{ Number, Optional }} }),
	Decl({ "Two",   Number, Left{{ Number, Optional }} }),
	Decl({ "Three", Number, Left{{ Number, Optional }} }),
	Decl({ "Four",  Number, Left{{ Number, Optional }} }),
	Decl({ "Five",  Number, Left{{ Number, Optional }} }),
	Decl({ "Six",   Number, Left{{ Number, Optional }} }),
	Decl({ "Seven", Number, Left{{ Number, Optional }} }),
	Decl({ "Eight", Number, Left{{ Number, Optional }} }),
	Decl({ "Nine",  Number, Left{{ Number, Optional }} }),
	// todo: these should probably be left associative not right associative
	// which could be implemented as implied skipping a continuous chain of numbers
	// but should probably be something else
	Decl({ "Divide", Number,
		Left{{ Number }},
		Right{{ Number }} }),
	Decl({ "Multiply", Number,
		Left{{ Number }},
		Right{{ Number }} }),
	Decl({ "Attribute_Value", Number,
		Right{{ Attribute_Type }} }),
	Decl({ "Group_Size_Of", Number,
		Right{{ Selector }} }),

	// Skip
	Decl({ "Skip", Skip }),

	// Termination
	Decl({ "Termination", Termination }),

	// Parameter_Reference
	// rmf todo: how to get this to pass type checking?
	// we could have one for each type...
	Decl({ "Parameter_Reference", Parameter_Reference })
};

const ElementDeclaration * ElementDictionary::GetDeclaration(ElementName name)
{
	if (declarations.count(name) == 0)
	{
		Error("Couldn't find ElementDeclaration for name " + name.value).Log();
		return nullptr;
	}
	return &declarations.at(name);
}

void ElementDictionary::GetAllowedNextElements(const NextTokenCriteria & criteria, std::set<ElementName> & out_allowed)
{
	out_allowed.clear();
	for (auto & pair : declarations)
	{
		auto & decl = pair.second;
		if (decl.left_parameter.get() == nullptr)
		{
			if (decl.types & criteria.validNextArgs)
			{
				out_allowed.insert(pair.first);
			}
		}
		else
		{
			// if we have an optional left parameter and fit arg types
			// we are still allowed
			if (decl.left_parameter->optional
				&& decl.types & criteria.validNextArgs)
			{
				out_allowed.insert(pair.first);
			}
			// or if we match the left parameter (optional or required)
			else if (decl.left_parameter->types & criteria.rightSideTypesForLeftParameter)
			{
				// our type is the same as the left_parameter
				if (decl.left_parameter->types == decl.types)
				{
					out_allowed.insert(pair.first);
				}
				// or if our type differs from the left_parameter
				else
				{
					// we need to check the mismatched list
					for (auto & mismatched : criteria.rightSideTypesForMismatchedLeftParameter)
					{
						// there could be multiple entries meeting the first
						// criteria, so we can't early out unless this type is allowed
						if (mismatched.first & decl.left_parameter->types
							&& mismatched.second & decl.types)
						{
							out_allowed.insert(pair.first);
							break;
						}
					}
				}
			}
		}
	}
}

} // namespace Command
