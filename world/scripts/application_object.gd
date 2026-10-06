## A session-local world entity; the manager owns its transient window binding.
extends CharacterBody3D

const APERTURE := Vector2(1.3, 0.82)
const BOUNDS := Vector3(1.4, 1.02, 0.08)
var entity_id := Crypto.new().generate_random_bytes(16).hex_encode()
var revision := 0
var texture: ImageTexture
var screen: MeshInstance3D
var label: Label3D
var rim: StandardMaterial3D
var housing: MeshInstance3D
var accent: MeshInstance3D
var bounds := BOUNDS
var gravity_active := false
var held := false
var presentation_hidden := false
var fall_speed := 0.0
var collider: CollisionShape3D
var support: WeakRef

func _physics_process(delta: float) -> void:
	if held or not gravity_active or not visible: return
	fall_speed = minf(fall_speed + 9.8 * delta, 12.0)
	# Sweep the whole panel, including its edges, rather than raycasting its center.
	# Keep probing after contact so removing a chair/support resumes the fall.
	support = null
	var motion := Vector3.DOWN * fall_speed * delta
	for attempt in range(3):
		var hit := move_and_collide(motion)
		if hit == null: break
		if hit.get_normal().y > 0.5:
			fall_speed = 0.0
			support = weakref(hit.get_collider())
			break
		motion = hit.get_remainder().slide(hit.get_normal())
		if motion.length_squared() < 0.000001: break

func _ready() -> void:
	collision_layer = 4
	collision_mask = 5 # World/furniture and other panels; never land on the player.
	safe_margin = 0.001
	collider = CollisionShape3D.new()
	collider.shape = BoxShape3D.new()
	(collider.shape as BoxShape3D).size = bounds
	add_child(collider)
	housing = MeshInstance3D.new()
	housing.mesh = BoxMesh.new()
	(housing.mesh as BoxMesh).size = BOUNDS
	var casing := StandardMaterial3D.new()
	casing.albedo_color = Color("172323")
	casing.metallic = 0.4
	casing.roughness = 0.4
	housing.material_override = casing
	add_child(housing)
	accent = MeshInstance3D.new()
	accent.mesh = BoxMesh.new()
	rim = StandardMaterial3D.new()
	rim.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	accent.material_override = rim
	add_child(accent)
	screen = MeshInstance3D.new()
	screen.mesh = QuadMesh.new()
	screen.material_override = preload("res://scripts/screen_material.gd").create()
	screen.position = Vector3(0, -0.045, 0.042)
	add_child(screen)
	label = Label3D.new()
	label.font_size = 28
	label.pixel_size = 0.0018
	label.position = Vector3(0, 0.445, 0.043)
	label.outline_size = 4
	add_child(label)
	visible = false

func show_frame(frame: Dictionary, shared_texture: ImageTexture = null) -> void:
	revision = frame.revision
	visible = frame.mapped
	collider.disabled = not visible
	if not visible:
		texture = null
		(screen.material_override as ShaderMaterial).set_shader_parameter("client_pixels", null)
		return
	if shared_texture != null:
		texture = shared_texture
	else:
		var image := preload("res://scripts/terminal_screen.gd").image_from_frame(frame)
		if texture == null or Vector2i(texture.get_size()) != image.get_size():
			texture = ImageTexture.create_from_image(image)
		else:
			texture.update(image)
	(screen.material_override as ShaderMaterial).set_shader_parameter("client_pixels", texture)
	var aspect := float(frame.logical_width) / float(frame.logical_height)
	var size := Vector2(APERTURE.y * aspect, APERTURE.y)
	if size.x > APERTURE.x: size = Vector2(APERTURE.x, APERTURE.x / aspect)
	(screen.mesh as QuadMesh).size = size
	bounds = Vector3(size.x + 0.08, size.y + 0.17, BOUNDS.z)
	(collider.shape as BoxShape3D).size = bounds
	(housing.mesh as BoxMesh).size = bounds
	(accent.mesh as BoxMesh).size = Vector3(bounds.x, 0.012, 0.012)
	accent.position = Vector3(0, bounds.y * 0.5 - 0.006, 0.04)
	label.position.y = bounds.y * 0.5 - 0.065

func select(selected: bool) -> void:
	rim.albedo_color = Color("d8b36c") if selected else Color("426460")

func set_presentation_hidden(hidden: bool) -> void:
	presentation_hidden = hidden
	for visual in [housing, accent, screen, label]: visual.visible = not hidden

func ray_distance(origin: Vector3, direction: Vector3) -> float:
	if not visible or presentation_hidden: return INF
	var local_origin := to_local(origin)
	var local_direction := global_basis.inverse() * direction
	# Only the visible front face is interactive; never activate through the back.
	if local_direction.z >= -0.00001: return INF
	var distance := (0.043 - local_origin.z) / local_direction.z
	var point := local_origin + local_direction * distance
	if distance <= 0 or absf(point.x) > bounds.x / 2 or absf(point.y) > bounds.y / 2:
		return INF
	return distance

func occlusion_distance(origin: Vector3, direction: Vector3) -> float:
	if not visible or presentation_hidden: return INF
	var hit = AABB(-bounds * 0.5, bounds).intersects_ray(to_local(origin), global_basis.inverse() * direction)
	return origin.distance_to(to_global(hit)) if hit != null else INF
