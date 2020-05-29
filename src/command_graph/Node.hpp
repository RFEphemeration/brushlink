struct TypeId
{
	int id;
};

struct TypeName
{
	std::string name;
};

struct MemberName
{
	std::string name;
};

enum class ValueType
{
	Int,
	Bool,
	Float,
	String,
	Struct

	// @Feature node/function values
	// @Feature array values
}

struct TypeInfo
{
	TypeName name;
	std::optional<TypeId> parent;
	ValueType value_type;

	// these are only set if the value_type is a struct
	// maybe they shouldn't necessarily be part of every typeinfo?
	std::vector<TypeId> child_types;
	std::unordered_map<MemberName, TypeId> members;

private:
	TypeId id; // only set by TypeLibrary

	friend struct TypeLibrary;
};

struct TypeLibrary
{
	static std::vector<TypeInfo> library {
		{ "Int", {}, ValueType::Int, {}, {}, 0 },
		{ "Bool", {}, ValueType::Bool, {}, {}, 1 },
		{ "Float", {}, ValueType::Float, {}, {}, 2 },
		{ "String", {}, ValueType::String, {}, {}, 3 },
	};

	static observer_ptr<TypeInfo> GetTypeInfo(TypeId id)
	{
		return &library[id.id];
	}

	static TypeId CreateType(TypeInfo&& info)
	{
		info.id = TypeId{library.size()};
		if (info.parent.has_value())
		{
			auto parent_type = GetTypeInfo(info.parent.value());
			parent_type.child_types.push_back(info.id);
		}
		library.emplace_back(std::move(info));
	}
};


using ValueBasic = std::variant<int, bool, float, std::string>;

template <typename ... TMembers>
using ValueRecord = std::tuple<TMembers...>;

struct Value
{
	TypeId type;
	std::optional<ValueBasic> basic_value;
	std::unordered_map<MemberName, Value> member_values;
	// @Cleanup what is a child_value? why do we have one?
	// std::optional<Value> child_value;

	bool IsBasic()
	{
		return basic_value.has_value();
	}

	template<typename T>
	ErrorOr<T&> GetAs()
	{
		if (basic_value.has_value()
			&& std::holds_alternative<T>(basic_value.value()))
		{
			return std::get<T>(basic_value.value());
		}
		else
		{
			return Error("This is not a basic type");
		}
	}

	template<typename T>
	ErrorOr<T&> GetMemberAs(MemberName name)
	{
		return CHECK_RETURN(GetMember(name)).GetAs<T>();
	}

	ErrorOr<Value&> GetMember(MemberName name)
	{
		if (Contains(member_values, name))
		{
			return member_values[name];
		}
		else
		{
			return Error("Member " + name.name + " not found");
		}
	}
};


struct NodeName
{
	std::string name;
}

struct Parameter
{
	bool required;
	bool repeatable;
	NodeName default_value;
};

// @Feature function vs procedure... purity
struct NodeDecl
{
	NodeName name;
	// @Feature Parameter struct that has implied, optional, default, repeatable
	std::vector<TypeInfo> results;
	std::vector<TypeInfo> left_parameters; // for infix operators
	std::vector<TypeInfo> parameters;

	value_ptr<NodeImpl> impl;
};

struct SavedValueName
{
	std::string name;
};

struct EvaluationContext
{
	observer_ptr<EvaluationContext> parent;

	// consider using observer_ptr
	std::optional<std::vector<Value>&> parameters;

	std::unordered_map<SavedValueName, Value> saved_values;

	ErrorOr<Success> SaveLocalValue(const SavedValueName & name, const Value & value)
	{
		if (parent == nullptr)
		{
			return Error("global context cannot have local values");
		}
		if (Contains(saved_values, name))
		{
			// @Feature shadowing variables is allowed at the moment... is that okay?
			return Error("saved local values cannot be overwritten");
		}
		saved_values[name] = value;
		return Success();
	}

	ErrorOr<Success> SaveGlobalValue(const SavedValueName & name, const Value & value)
	{
		if (Contains(saved_values, name))
		{
			return Error("global values cannot have the same name as locals");
		}
		if (parent != nullptr)
		{
			return Parent->SaveGlobalValue(name, value);
		}
		saved_values[name] = value;
		return Success();
	}
}

struct NodeImpl
{
	observer_ptr<NodeDecl> declaration;

	virtual ErrorOr<std::vector<Value>> Evaluate(
		EvaluationContext & context,
		const std::vector<Value> & arguments) const;
};

struct NodeImplLiteral : NodeImpl
{
	std::vector<Value> values;

	virtual ErrorOr<std::vector<Value>> Evaluate(
		EvaluationContext & context,
		const std::vector<Value> & arguments) const override
	{
		if (arguments.size() > 0)
		{
			return Error("Literals should not have arguments");
		}
		return values;
	}
};

struct NodeImplFunc : NodeImpl
{
	ErrorOr<std::vector<Value>> (*func)(EvaluationContext & context, const std::vector<Value> &);

	virtual ErrorOr<std::vector<Value>> Evaluate(
		EvaluationContext & context,
		const std::vector<Value> & arguments) const override
	{
		return func(context, arguments);
	}
};

struct NodeImplContextFunc : NodeImpl
{
	ErrorOr<std::vector<Value>> (EvaluationContext::*func)(const std::vector<Value> &);

	virtual ErrorOr<std::vector<Value>> Evaluate(
		EvaluationContext & context,
		const std::vector<Value> & arguments) const override
	{
		return context.*func(arguments);
	}
};

struct NodeImplNode : NodeImpl
{
	Node root;

	virtual ErrorOr<std::vector<Value>> Evaluate(
		EvaluationContext & context,
		const std::vector<Value> & arguments) const override
	{
		EvaluationContext child_context {context, arguments};
		return root.Evaluate(child_context);
	}
};

// @Feature implied Node to use only some of the arguments

enum class Implied
{
	Explicit, // This has been created / confirmed by a user
	Parent, // Implied Parents are usually type conversions
	Child, // Implied Children are a way of making pseudo NodeImplNode
	Default, // Default has the lowest priority
	Suggestion // Suggestions are active in the current search
}


struct Node
{
	// should implied status be part of this? none, parent, child?
	Implied implied;
	observer_ptr<NodeDecl> decl;
	std::vector<value_ptr<Node>> left_arguments;
	std::vector<value_ptr<Node>> arguments;

	ErrorOr<std::vector<Value>> Evaluate(EvaluationContext & context) const
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
};

// Node -> NodeImplNode process
/*
at first just does it, creates new NodeImplNode with no parameters
that NodeImplNode needs a new NodeDecl, which can be owned by the Node (anonymouse lambda)
but later given a name to be added to the Library
can turn child nodes into parameters, which adds another parameter and moves the corresponding node to being an argument
should be able to do this for default values, also
*/

