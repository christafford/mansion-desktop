## Upright movable furniture; imported triangles are visual geometry only.
extends RigidBody3D

func _ready() -> void:
	mass = 7.0
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
	# Fitted to the source mesh in meters: seat at .40–.45, back to .975.
	shape(Vector3(0.42, 0.09, 0.44), Vector3(0, 0.405, 0.015))
	shape(Vector3(0.42, 0.54, 0.105), Vector3(0, 0.705, -0.212), -0.23)
	for x in [-0.16, 0.16]:
		for z in [-0.18, 0.19]:
			shape(Vector3(0.045, 0.37, 0.045), Vector3(x, 0.186, z))

func shape(size: Vector3, center: Vector3, pitch := 0.0) -> void:
	var collision := CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = size
	collision.shape = box
	add_child(collision)
	collision.position = center
	collision.rotation.x = pitch

func push(walking_velocity: Vector3, normal: Vector3, delta: float) -> void:
	var direction := -normal * Vector3(1, 0, 1)
	if direction.length_squared() < 0.01: return
	direction = direction.normalized()
	var relative_speed := maxf(0, (walking_velocity - linear_velocity).dot(direction))
	# Only impart the missing walking speed; repeated contact cannot launch it.
	apply_central_impulse(direction * mass * minf(relative_speed, 1.5) * minf(1, delta * 10))
