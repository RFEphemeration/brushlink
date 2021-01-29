#include "Player.h"

namespace Brushlink
{

Player Player::FromSettings(const Player_Settings & settings, PlayerID id, Point starting_location)
{
	std::shared_ptr<Player_Data> data {new Player_Data{}}; // @Feature LoadData
	Player p{
		data,
		settings,
		id,
		data->graphical_preferences.front(),
		starting_location,
		Point{0,0}, // camera location, is this bottom_left or center?
		Command::Context{}, // @Feature Commands
		{}
	};
	return p;
}

void Player::RemoveUnits(Set<UnitID> unit_ids)
{
	// todo: remove from command context stored variables
	// and from per-tick evaluations, if necessary
	for (auto & pair : command_groups)
	{
		for (auto & id : unit_ids)
		{
			pair.second.units.remove(id);
		}
	}
}

ErrorOr<ElementToken> Player::GetTokenForName(ElementName name)
{
	if (Contains(exposed_elements, name))
	{
		return {name, exposed_elements[name]->type};
	}
	if (Contains(hidden_elements, name))
	{
		return {name, hidden_elements[name]->type};
	}
	if (Contains(builtins, name))
	{
		return {name, builtins[name]->type};
	}
	return Error{"No element with name " + name.value + " was found."};
}

} // namespace Brushlink
