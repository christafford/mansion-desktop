## Upright movable furniture; imported triangles are visual geometry only.
extends RigidBody3D

var kind := "dining_chair_02"
# Authored assemblies add fitted convex shapes after construction.
var assembly_mass := 12.0
var model_scale := 1.0
var placement_box := AABB()
var saved_linear := Vector3.ZERO
var saved_angular := Vector3.ZERO

func measure_visuals() -> void:
	var first := true
	for mesh in find_children("*", "MeshInstance3D", true, false):
		var bounds: AABB = global_transform.affine_inverse() * mesh.global_transform * mesh.get_aabb()
		placement_box = bounds if first else placement_box.merge(bounds)
		first = false

func begin_hold() -> void:
	saved_linear = linear_velocity
	saved_angular = angular_velocity
	freeze = true
	linear_velocity = Vector3.ZERO
	angular_velocity = Vector3.ZERO

func end_hold(cancel: bool) -> void:
	linear_velocity = saved_linear if cancel else Vector3.ZERO
	angular_velocity = saved_angular if cancel else Vector3.ZERO
	freeze = false
	sleeping = false

func _ready() -> void:
	collision_mask = 3 # Room and player; panels probe/support themselves.
	mass = assembly_mass if kind == "assembly" else (25.0 if kind == "wooden_bookshelf_worn" else 7.0)
	linear_damp = 3.0
	angular_damp = 5.0
	axis_lock_angular_x = true
	axis_lock_angular_z = true
	continuous_cd = true
	add_to_group("pushable_furniture")
	var surface := PhysicsMaterial.new()
	surface.friction = 0.45
	surface.bounce = 0.0
	physics_material_override = surface
	if kind == "wooden_bookshelf_worn":
		for x in [-0.66, 0.66]:
			shape(Vector3(0.054, 2.0634, 0.58), Vector3(x, 1.0317, 0))
		shape(Vector3(1.27, 2.02, 0.025), Vector3(0, 1.03, -0.275))
		for y in [0.04, 0.44, 0.84, 1.24, 1.64, 2.0434]:
			shape(Vector3(1.32, 0.04, 0.57), Vector3(0, y, 0))
	elif kind == "potted_plant_02":
		var pot := CollisionShape3D.new()
		var cylinder := CylinderShape3D.new()
		cylinder.radius = 0.232 * model_scale
		cylinder.height = 0.336 * model_scale
		pot.shape = cylinder
		add_child(pot)
		pot.position.y = 0.168 * model_scale
	elif kind == "dining_chair_02":
		# Fitted to the source mesh in meters: seat at .40–.45, back to .975.
		shape(Vector3(0.42, 0.09, 0.44), Vector3(0, 0.405, 0.015))
		shape(Vector3(0.42, 0.54, 0.105), Vector3(0, 0.705, -0.212), -0.23)
		for x in [-0.16, 0.16]:
			for z in [-0.18, 0.19]:
				shape(Vector3(0.045, 0.37, 0.045), Vector3(x, 0.186, z))

func shape(size: Vector3, center: Vector3, pitch := 0.0) -> void:
	var collision := CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = size * model_scale
	collision.shape = box
	add_child(collision)
	collision.position = center * model_scale
	collision.rotation.x = pitch

func push(walking_velocity: Vector3, normal: Vector3, delta: float) -> void:
	var direction := -normal * Vector3(1, 0, 1)
	if direction.length_squared() < 0.01: return
	direction = direction.normalized()
	var relative_speed := maxf(0, (walking_velocity - linear_velocity).dot(direction))
	# Only impart the missing walking speed; repeated contact cannot launch it.
	apply_central_impulse(direction * mass * minf(relative_speed, 1.5) * minf(1, delta * 10))
