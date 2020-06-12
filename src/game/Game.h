

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
	Map<PlayerID, PlayerSettings> player_settings;
	Map<Unit_Type, UnitSettings> unit_types;
	Ticks speed {12}; // per second

	static const GameSettings default_settings;
};

struct Game
{
	GameSettings settings;
	World world;
	Map<PlayerID, Player> players;

	UnitID next_unit_id;
	Ticks tick;

	void Tick();
	// helper functions for Tick
	void ProcessPlayerInput();
	void RunPlayerCoroutines();
	void ApplyCrowdingDecay();
	void AllUnitsTakeAction();
	Action_Result UnitTakeAction(Unit & unit);
	void PruneExhaustedUnits();

	ErrorOr<UnitID> SpawnUnit(PlayerID player, Unit_Type unit, Point position);

	Ticks SecondsToTicks(Seconds s);
	
};