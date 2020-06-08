extends Node


enum BillboardStyle {
	CameraVertical, # for vertical or fixed tilted pieces
	CameraComplete, # to face the camera completely, for projectiles?
	WorldVerticalX,
	WorldVerticalZ,
}

const z_out_of_bounds := -100000
const x_out_of_bounds := 100000

export(BillboardStyle) var style = BillboardStyle.CameraVertical

onready var skeleton : Skeleton = $"../Armature/Skeleton"
onready var anim_tree : AnimationTree = $"../AnimationTree"
onready var anim_playback : AnimationNodeStateMachinePlayback = anim_tree.get("parameters/playback")
onready var bones : Array = get_board_bones()

export var board_every_frame : bool = false

func _ready():
	anim_tree.active = true
	anim_playback.travel("Awake")

func _process(delta):
	if board_every_frame:
		board_bones(delta)

func get_board_bones():
	var board_bones := []
	for i in skeleton.get_bone_count():
		if skeleton.get_bone_name(i).find("board") != -1:
			board_bones.append(i)
	return board_bones

func set_board_every_frame(var board):
	if style == BillboardStyle.CameraVertical or style == BillboardStyle.CameraComplete:
		board_every_frame = board
	else:
		if board:
			board_bones(1.0)
		board_every_frame = false

func board_bones(delta):
	# @Feature tween boarding
	#var amount := 1.0
	var camera_pose := get_viewport().get_camera().get_camera_transform()
	for i in bones:
		var bone_pose := skeleton.get_bone_global_pose(i)
		var target : Vector3 = bone_pose.origin
		match style:
			BillboardStyle.CameraVertical:
				target = camera_pose.origin
				target.y = bone_pose.origin.y
			BillboardStyle.CameraComplete:
				target = camera_pose.origin
			BillboardStyle.WorldVerticalX:
				target = bone_pose.origin
				target.x = x_out_of_bounds
			BillboardStyle.WorldVerticalZ:
				target = bone_pose.origin
				target.z = z_out_of_bounds
		var board_pose := bone_pose.looking_at(target, Vector3.UP) * Transform.IDENTITY.rotated(Vector3.LEFT, PI / 2)
		# var board_pose := bone_pose.rotated(Vector3.UP, delta)
		skeleton.set_bone_global_pose_override(i, board_pose, 1.0, true)
