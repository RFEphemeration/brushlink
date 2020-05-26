extends Spatial

class_name Move

const repath_time : float = 3.0
const destination_threshold : float = 0.1

export var move_speed : float = 4.0
export var acceleration_time = 0.2
export var rot_spring_time : float = 0.33

onready var animation_tree = get_node("./graphics/body/AnimationTree")
onready var unit = get_node("../")
onready var navigation_mesh = get_node("../../")
onready var next_target_distance = move_speed * get_physics_process_delta_time() * 10

var current_speed : float = 0.0
var targets : Array = []
var time_until_repath : float = 0.0

# Considerations:
"""
Do we want to retry pathfinding every once and a while in order to guarantee that we will arrive?
How often? or should we check only under specific conditions? tracking them feels challenging
"""

func use(destination : Vector2):
	# @Feature pathfinding
	var current = unit.get_global_transform().get_translation()
	pathfind_to(Vector3(destination.x, current.y, destination.y))

func pathfind_to(destination : Vector3):
	var current = unit.get_global_transform().get_translation()
	targets = navigation_mesh.get_simple_path(current, destination)
	time_until_repath = repath_time

func _physics_process(delta : float):
	if targets.empty():
		return
	time_until_repath -= delta
	if (time_until_repath <= 0.0):
		pathfind_to(targets[-1])
	if (current_speed < move_speed):
		# and we aren't within some distance from our final destination
		current_speed += move_speed / acceleration_time * delta
		animation_tree.set("parameters/MoveSpeed/blend_amount", current_speed / move_speed)
	
	var velocity : Vector3
	
	var current = unit.get_global_transform().get_translation()
	var difference : Vector3 = (targets[0] - current)
	var distance : float = difference.length()
	if (targets.size() > 1 && distance < next_target_distance):
		targets.pop_front()
		difference = (targets[0] - current)
		distance = difference.length()
		velocity = difference / distance * current_speed
	elif (targets.size() == 1 && distance < destination_threshold):
		targets.pop_front()
		finished()
		return
	elif (targets.size() == 1 && distance < current_speed * delta):
		# @Feature slow to a stop? this is instantaneous
		velocity = difference / delta
	else:
		velocity = difference / distance * current_speed
		
	unit.move_and_slide_with_snap(velocity, Vector3.DOWN, Vector3.UP)
	unit.transform.interpolate_with(
		unit.transform.looking_at(targets[0], Vector3.UP),
		delta / rot_spring_time)
	
	unit.transform = unit.transform.orthonormalized()

func finished():
	# @Feature, cross fade out of movement to idle
	animation_tree.set("parameters/MoveSpeed/blend_amount", 0.0)
	# @Incomplete send event that we've finished moving
	current_speed = 0.0
	
