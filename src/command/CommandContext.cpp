
ErrorOr<Success> CommandContext::HandleToken(ElementToken token)
{

	// trying out the simplest grammar to implement
	// which means no default variables
	// no left parameters

	if (command != nullptr)
	{
		if (command->ParameterCount() >= 1
			&& command->ParameterType<1>() != token.type)
		{
			return Error("Token is of unexpected type");
		}
		else if (command->ParameterCount() == 0
			&& token.type != Confirmation
			&& token.type != Cancel)
		{
			return Error("The Command is complete, waiting for Confirmation or Cancel only");
		}
	}
	else if (command == nullptr)
	{
		if (token.type == Action)
		{
			if (actors.IsUnset())
			{
				actors = CurrentSelection();
			}
		}
		else if (IsSelectorSubtype(token.type))
		{
			command = new Functor(SetActors);
		}
		else
		{
			return Error("Cannot start a command with anything other than an action or actor selector");
		}
	}

	// we are still working on the actors
	// or about to take a command
	
		switch(token.type)
		{
			case Action:
				Functor * action;
				switch(token.name)
				{
					case "SetCurrentSelection":
						action = new CurriedFunctor(SetCurrentSelection, actors);
						break;
					case "Move":
						action = new CurriedFunctor(Move, actors);
						break;
					case "Attack":
						action = new CurriedFunctor(Attack, actors);
						break;
				}
				if (command == nullptr)
				{
					command = action;
				}
				else
				{
					// todo: ordering?
					command = ComposedFunctor(action, command);
				}
				break;
			case Selector_Base:
				switch(token.name)
				{
					case "Enemies":
						actors = Enemies();
						break;
					case "Allies":
						actors = Allies();
						break;
					case "CurrentSelection":
						actors = CurrentSelection();
						break;
					case "Actors":

						break;
				}
				break;
			case Selector_Filter:
				switch(token.name)
				{
					case "WithinActorsRange":
						return Error("WithinActorsRange is not a valid filter during Actor selection");
					case "OnScreen":
						if (actors.IsEmpty())
						{
							actors_selector = OnScreen(Allies());
						}
						else
						{
							actors_selector = OnScreen(actors);
						}
				}
				break;
			case Selector_Superlative:
				switch(token.name)
				{
					case "ClosestToActors":
						return Error("ClosestToActors is not a valid filter during Actor selection");
				}
				break;
			default:
				// todo: conditionals
				return Error("Only selectors and actions are allowed to initiate commands");
		}
	}


	if (actors_selector != nullptr
		&& actors_selector->ParameterCount() == 1
		&& actors_selector->ParameterType<1>() != UnitGroup)
	{
		actors = actors_selector(actors);
		destroy(actors_selector);
		actors_selector = nullptr;
	}

	if (command != nullptr
		&& command->ParameterCount() == 0)
	{
		command();
		destroy(command);
	}
}