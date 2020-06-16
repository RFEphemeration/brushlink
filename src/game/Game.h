

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
	Number crowded_threshold {6}; // number of neighbors at which we start decaying
	std::pair<Number, Seconds> crowded_decay{{1}, {1.0}};

	static const GameSettings default_settings;
};

struct Game
{
	GameSettings settings;
	World world;
	Map<PlayerID, Player> players;

	std::random_device random_seed; // seed
	std::mt19937 random_generator; // mersene twister from seed

	UnitID next_unit_id;
	Ticks tick;

	void Initialize();

	void Tick();
	// helper functions for Tick
	void ProcessPlayerInput();
	void RunPlayerCoroutines();
	void AllUnitsTakeAction();
	Action_Result UnitTakeAction(Unit & unit);
	void ApplyCrowdingDecayAndPruneExhaustedUnits();

	ErrorOr<UnitID> SpawnUnit(PlayerID player, Unit_Type unit, Point position);
	void RemoveUnits(Set<UnitID> units);

	Ticks SecondsToTicks(Seconds s);
	
};