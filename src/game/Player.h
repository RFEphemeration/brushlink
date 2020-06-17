#pragma once
#ifndef BRUSHLINK_PLAYER_H
#define BRUSHLINK_PLAYER_H

#include <vector>

#include "NamedType.hpp"

#include "Player_Graphics.h"

namespace Brushlink
{

struct PlayerIDTag
{
	static HString GetName() { return "PlayerID"; }
};
using PlayerID = NamedType<int, PlayerIDTag>;

struct Player
{
	std::vector<Player_Graphics> graphical_preferences;
};


} // namespace Brushlink

#endif // BRUSHLINK_PLAYER_H

