

enum class Player_Type
{
	AI,
	Local_Player,
	// Todo?: Remote_Player
};

struct PlayerSettings
{
	Player_Type type;
	// Todo: dictionary of command elements
};

struct GameSettings
{
	Map<PlayerID, PlayerSettings> player_settings {
		{{0}, {
			Player_Type.Local_Player}
		},
		{{1}, {
			Player_Type.AI}
		},
	};
	Map<Unit_Type, UnitSettings> unit_types {
		{ Unit_Type.Spawner, {
			Unit_Type.Spawner,
			{6}, {24}, {{1}, {0.5}}, // more energy in order to reduce healing and make harder to kill
			{}, // todo: drawn_body
			{
				{ Action_Type.Nothing, Action_Settings{} },
				{ Action_Type.Idle, Action_Settings{} },
				{ Action_Type.Move, Action_Settings{{0}, {0}, {0.5}, {0.25}} },
				{ Action_Type.Reproduce, Action_Settings{{15}, {0}, {4.0}, {1.0}} },
			}
		}},
		{ Unit_Type.Healer, {
			Unit_Type.Healer,
			{8}, {12}, {{1}, {1.0}}, // starts at moderate health, mild regen
			{}, // todo: drawn_body
			{
				{ Action_Type.Nothing, Action_Settings{} },
				{ Action_Type.Idle, Action_Settings{} },
				{ Action_Type.Move, Action_Settings{{0}, {0}, {2.0 / 3.0}, {1.0 / 3.0}} },
				{ Action_Type.Heal, Action_Settings{{2}, {3}, {1.5}, {1.0 / 6.0}} },
				// todo: heal action target modifier so healing a healer is 1-1
			}
		}},
		{ Unit_Type.Attacker, {
			Unit_Type.Attacker,
			{12}, {12}, {{1}, {6.0}}, // starts at full health, very slow regen
			{}, // todo: drawn_body
			{
				{ Action_Type.Nothing, Action_Settings{} },
				{ Action_Type.Idle, Action_Settings{} },
				{ Action_Type.Move, Action_Settings{{0}, {0}, {1.0 / 3.0}, {1.0 / 6.0}} },
				{ Action_Type.Attack, Action_Settings{{0}, {3}, {1.0}, {1.0 / 6.0}} },
			}
		}},
	};
	Ticks speed {12}; // per second
};


const GameSettings default_game_settings;

struct Game
{
	GameSettings settings;
	World world;
	Map<PlayerID, Player> players;

	UnitID next_unit_id;
	Ticks tick;

	void Tick();

	ErrorOr<UnitID> SpawnUnit(PlayerID player, Unit_Type unit, Point position);
	
};