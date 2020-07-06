# brushlink

So far this is just a wip command language for an rts game. The rest of the game might spring up around it at some future date.

Makes use of an in progress game engine and tool set of mine: [farb](https://github.com/RFDaemoniac/farb).

The command language is function driven, with a fair number of builtins and user definable functions available. The AST is built directly in game using a gui / hotkeys. User definable functions will probably be able to be loaded from text, but this is not yet implemented.

The AST is made up of CommandElement nodes, which can have some number of CommandParameter edges. These edge objects define the restrictions for which CommandElement children are allowed.

CommandElement types are as follows:\
```
  Command
  Action
  Selector // always implied, there to define parameters of the following 4
    Set
    Filter
    GroupSize
    Superlative
  Point
  Line
  Area
  Location // an alternate name for OneOf<Point, Line, Area>
  UnitType
  Attribute
  Ability
  Resource
  
  Number
  Condition
  
  Skip // used to skip optional parameters of the same type as the proceeding Element
  Cancel
  Termination/Execute
```
  
A sample command sequence might look like:
```
  Attack CommandGroup 2 WithinRange 2 ActorRatio 3 HighestAttribute Energy Termination
```
which gets interpreted as, with () nodes implied:\
```
  (Command)
    Attack
      CommandGroup // these become the Actors for the Action Attack
        2
      (Selector)
        (Enemies)
        WithinRange
          2 // this is a modifier, so it's unit range + 2
        ActorRatio // this is a GroupSize based on the number of Actors
          3 // there should be one Target for every 3 Actors
        HighestAttribute // if there were no ActorRatio, adding a Superlative would imply a single target
          Energy  // if there are more units WithinRange than we want, prioritize those with the most energy
```
