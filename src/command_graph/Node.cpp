

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
	const std::vector<TypeInfo> & paramters,
	const std::vector<value_ptr<Node>> & arguments,
	const std::vector<TypeInfo> & pending)
{
	// @Feature change parameters from TypeInfo to Parameter
	// @Feature repeatable, optional
	// @Bug multiple return values don't line up here
	// because we assume arguments.size matches parameters.size
	int first_index = arguments.size();
	// @Feature dropping unused extra return values?
	if (paramters.size() < first_index + pending.size())
	{
		return false;
	}
	for (int i = 0; i < pending.size(); i++)
	{
		if (!parameters[first_index + i].MatchesOrIsParentOf(context, pending[i]))
		{
			return false;
		}
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
			arg->decl->results))
	{
		left_arguments.emplace_back(arg);
		arg_location.push_back(&left_arguments.back());
		return arg_location;
	}

	if (IsPendingArgumentValid(
			context,
			decl->parameters,
			arguments,
			arg->decl->results))
	{
		arguments.emplace_back(arg);
		arg_location.push_back(&arguments.back());
		return arg_location;
	}

	return Error("Invalid argument");
}
