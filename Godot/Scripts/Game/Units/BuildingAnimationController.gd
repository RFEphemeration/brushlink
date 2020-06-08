extends Node

onready var anim_tree : AnimationTree = $"../AnimationTree"
onready var anim_player : AnimationPlayer = $"../AnimationPlayer"
onready var anim_playback : AnimationNodeStateMachinePlayback = anim_tree.get("parameters/playback")
onready var model : MeshInstance = $"../Armature/Skeleton/Model"
var active := false

export var wait_for_idle_point : bool = true

func _ready():
	anim_tree.tree_root.set_parameter("active/activity", 0.0)
	construct()

func set_active():
	active = true
	anim_playback.travel("active")
	anim_tree.tree_root.set_parameter("active/activity", 1.0)
	
func set_inactive():
	active = false
	if not wait_for_idle_point:
		potential_idle_point()
	
func construct():
	anim_playback.start("construction")

# @Feature buildings destroyed during construction

func destruct():
	var construction_amount := 1.1
	if anim_playback.get_current_node() == "construction":
		construction_amount = anim_player.get_current_animation_position() \
			/ anim_player.get_current_animation_length() 
	anim_tree.tree_root.set_parameter("active/activity", 0.0)
	anim_playback.start("destruction")
	if construction_amount < 1.0:
		var destruction_anim = anim_tree.tree_root.get_node(anim_playback.get_current_node()).animation
		anim_player.advance(destruction_anim.length * construction_amount)

func potential_idle_point():
	if not active:
		anim_tree.tree_root.set_parameter("active/activity", 0.0)
		anim_playback.travel("idle")

func make_material_override_unique():
	model.set_material_override(model.material_override.duplicate())

func destruction_animation_complete():
	pass
