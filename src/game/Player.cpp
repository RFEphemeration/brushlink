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
		CommandContext{} // @Feature Commands
	};
	return p;
}

void Player::RemoveUnits(Set<UnitID> unit_ids)
{
	// todo: remove from command context stored variables
	// and from per-tick evaluations, if necessary
}

} // namespace Brushlink
