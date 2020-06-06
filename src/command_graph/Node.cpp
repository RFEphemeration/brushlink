

bool TypeInfo::MatchesOrIsParentOf(const EvaluationContext & context, const TypeInfo & other) const
{
	if (name == other.name)
	{
		return true;
	}

	const TypeInfo * type = &other;
	// @Feature multiple inheritence? probably don't
	while (type->parent.has_value())
	{
		if (name == type->parent.value())
		{
			return true;
		}
		type = context.GetType(type->parent.value());
	}
	return false;
}

ErrorOr<std::vector<Value>> Node::Evaluate(EvaluationContext & context) const
{
	std::vector<Value> values;
	for (auto & arg : left_arguments)
	{
		auto arg_values = CHECK_RETURN(arg->Evaluate());
		values.insert(left_values.end(), arg_values.begin(), arg_values.end());
	}
	for (auto & arg : arguments)
	{
		auto arg_values = CHECK_RETURN(arg->Evaluate());
		values.insert(values.end(), arg_values.begin(), arg_values.end());
	}
	return decl->impl->Evaluate(context, values);
}

int IsPendingArgumentValid(
	const EvaluationContext & context,
	const std::vector<TypeInfo> & parameters,
	const std::vector<Node> & arguments,
	const Node & pending)
{
	// @Feature change parameters from TypeInfo to Parameter
	// @Feature splitting multiple return values between different nodes
	int param_index = -1;
	for (int arg_index = 0;
		param_index < parameters.size() && arg_index < arguments.size();
		arg_index++)
	{
		auto arg_types = arguments[arg_index]->decl->results;
		for (int i = 0; i < arg_types.size(); i++)
		{
			// @Feature repeatable, optional
			// @Feature skips, maybe arguments need to be handled differently
			param_index++;
		}
	}

	if (param_index >= parameters.size() - 1)
	{
		return false;
	}

	for (int i = 0;
		param_index < parameters.size() && i < pending->decl->results.size();
		i++)
	{
		const auto & type = pending->decl->results[i];
		if (!parameters[param_index].MatchesOrIsParentOf(context, type))
		{
			return false;
		}
		// @Feature repeatable, optional, skips
		param_index++;
	}
	// we expect param_index to match size here because we increment at the end of the loop
	if (param_index > parameters.size())
	{
		return false;
	}

	return true;
}

ErrorOr<std::vector<Node *>> Node::SetArgument(
	const EvaluationContext & context,
	Node && arg,
	Node::SetArgFlags flags /* = DefaultSetArg */)
{
	std::vector<Node *> arg_location;
	if (flags & Node::RecurseOnArgumentsFirst)
	{
		Node * potential_location = nullptr;
		ErrorOr<std::vector<Node *>> result = [&]{
			// can only append to last argument for now.
			if (!arguments.empty())
			{
				potential_location = &arguments.back()
				return arguments.back()->SetArgument(arg, flags);
			}
			else if (flags & Node::AllowSetLeft && !left_arguments.empty())
			{
				potential_location = &left_arguments.back()
				return left_arguments.back()->SetArgument(arg, flags);
			}
			return Error("No arguments to recurse on");
		}();
		
		if (!result.IsError())
		{
			arg_location.push_back(potential_location)
			arg_location.insert(
				arg_location.end(),
				result.GetValue().begin(),
				result.GetValue().end()
			);
			return arg_location;
		}
	}
	if (flags & Node::AllowSetLeft
		&& IsPendingArgumentValid(
			context,
			decl->left_parameters,
			left_arguments,
			arg))
	{
		left_arguments.emplace_back(std::move(arg));
		arg_location.push_back(&left_arguments.back());
		return arg_location;
	}

	if (IsPendingArgumentValid(
			context,
			decl->parameters,
			arguments,
			arg))
	{
		arguments.emplace_back(std::move(arg));
		arg_location.push_back(&arguments.back());
		return arg_location;
	}

	return Error("Invalid argument");
}
