

enum class Player_Type
{
	AI,
	Local_Player
};

struct PlayerSettings
{
	Player_Type type;
	// Todo: dictionary of command elements
};

struct GameSettings
{
	Map<PlayerID, PlayerSettings> player_settings {
		{PlayerID{0}, PlayerSettings{
			Player_Type.Local_Player}
		},
		{PlayerID{1}, PlayerSettings{
			Player_Type.AI}
		},
	};
	Map<Unity_Type, UnitSettings> unit_types;
	Ticks speed {12}; // per second
};


const GameSettings default_game_settings;

struct Game
{
	GameSettings game_settings;
	World world;
	Map<PlayerID, Player> players;

	UnitID next_id;

};