extends Node

onready var anim_tree : AnimationTree = $"../AnimationTree"
onready var anim_playback = anim_tree.get("parameters/playback")
onready var model : MeshInstance = $"../Armature/Skeleton/Model"
var active := false

func _ready():
	anim_tree.tree_root.set_parameter("activity/active", 0.0)
	construct()

func set_active():
	active = true
	anim_tree.tree_root.set_parameter("activity/active", 1.0)
	anim_tree.active = true
	
func set_inactive():
	active = false

func potential_stop_point():
	if not active:
		anim_tree.tree_root.set_parameter("activity/active", 0.0)
		anim_tree.active = false

func construct():
	anim_playback.start("construction")

func destruct():
	anim_tree.tree_root.set_parameter("activity/active", 0.0)
	anim_playback.travel("destruction")

func make_material_override_unique():
	model.set_material_override(model.material_override.duplicate())

func destruction_animation_complete():
	pass
