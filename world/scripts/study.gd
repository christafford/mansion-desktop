## Furnished study with a live, unlit Wayland terminal screen.
extends Node3D

# Retain the study's test-facing accessors; the desktop node owns the services.
@onready var desktop_runtime: Node = $DesktopRuntime
var terminal_screen: Node:
	get: return desktop_runtime.terminal_screen
var application_mode: CanvasLayer:
	get: return desktop_runtime.application_mode
var app_launcher: CanvasLayer:
	get: return desktop_runtime.app_launcher
var application_objects: Node3D:
	get: return desktop_runtime.application_objects

var furniture: Array[Node3D] = []

func material(color: Color, texture_path: String = "", uv: float = 1.0) -> StandardMaterial3D:
	var result := StandardMaterial3D.new()
	result.albedo_color = color
	result.roughness = 0.8
	if not texture_path.is_empty():
		result.albedo_texture = load(texture_path)
		result.uv1_scale = Vector3(uv, uv, uv)
	return result

func box(label: String, size: Vector3, pos: Vector3, mat: Material, solid: bool = true, parent: Node3D = self) -> MeshInstance3D:
	var node := MeshInstance3D.new()
	node.name = label
	var mesh := BoxMesh.new()
	mesh.size = size
	node.mesh = mesh
	node.material_override = mat
	parent.add_child(node)
	node.position = pos
	if solid:
		var body := StaticBody3D.new()
		var shape := CollisionShape3D.new()
		var bounds := BoxShape3D.new()
		bounds.size = size
		shape.shape = bounds
		body.add_child(shape)
		node.add_child(body)
	return node

func model(label: String, asset: String, pos: Vector3, yaw: float = 0, movable := false) -> Node3D:
	var scene: PackedScene = load("res://assets/" + asset + ".gltf")
	assert(scene != null, "Missing imported model: " + asset)
	var node := scene.instantiate() as Node3D
	node.name = label
	if movable:
		var body := preload("res://scripts/pushable_furniture.gd").new()
		body.name = label
		body.kind = asset
		add_child(body)
		body.position = pos
		body.rotation.y = yaw
		body.add_child(node)
		body.measure_visuals()
		furniture.append(body)
		return body
	add_child(node)
	node.position = pos
	node.rotation.y = yaw
	furniture.append(node)
	for mesh in node.find_children("*", "MeshInstance3D", true, false):
		mesh.create_trimesh_collision()
	return node

func _ready() -> void:
	var wood := material(Color(0.8, 0.73, 0.62), "res://assets/textures/walnut_veneer_4k_jpg.jpg", 3)
	var panel_backing := material(Color("30271f"))
	var trim := material(Color(0.18, 0.22, 0.2))
	box("Floor", Vector3(8, 0.16, 9), Vector3(0, -0.08, 0), wood)
	box("BackWall", Vector3(8, 3.2, 0.16), Vector3(0, 1.6, -4.5), panel_backing)
	# Opening opposite the desk: no hidden wall or skirting across the threshold.
	for x in [-2.425, 2.425]:
		box("FrontWallSide", Vector3(3.15, 3.2, 0.16), Vector3(x, 1.6, 4.5), panel_backing)
	box("FrontWallHeader", Vector3(1.7, 0.7, 0.16), Vector3(0, 2.85, 4.5), panel_backing)
	box("RightWall", Vector3(0.16, 3.2, 9), Vector3(4, 1.6, 0), panel_backing)
	# A real opening in the left wall admits daylight.
	box("LeftWallLower", Vector3(0.16, 0.85, 9), Vector3(-4, 0.425, 0), panel_backing)
	box("LeftWallUpper", Vector3(0.16, 0.6, 9), Vector3(-4, 2.9, 0), panel_backing)
	box("LeftWallBack", Vector3(0.16, 1.75, 2.6), Vector3(-4, 1.725, -3.2), panel_backing)
	box("LeftWallFront", Vector3(0.16, 1.75, 3.4), Vector3(-4, 1.725, 2.8), panel_backing)
	var paneling := preload("res://scripts/wood_paneling.gd").new()
	paneling.name = "WoodPaneling"
	add_child(paneling)
	box("Ceiling", Vector3(8, 0.12, 9), Vector3(0, 3.26, 0), material(Color(0.9, 0.87, 0.8)))
	box("Skirting", Vector3(7.8, 0.14, 0.06), Vector3(0, 0.07, -4.39), trim, false)
	for x in [-2.425, 2.425]:
		box("EntrySkirting", Vector3(3.05, 0.14, 0.06), Vector3(x, 0.07, 4.39), trim, false)
	var hallway := preload("res://scripts/hallway.gd").new()
	hallway.name = "Hallway"
	add_child(hallway)
	for x in [-3.89, 3.89]:
		box("Skirting", Vector3(0.06, 0.14, 8.8), Vector3(x, 0.07, 0), trim, false)
	box("WindowSill", Vector3(0.4, 0.08, 3.2), Vector3(-3.88, 0.89, -0.4), trim)
	for z in [-1.9, -0.4, 1.1]:
		box("WindowMullion", Vector3(0.16, 1.75, 0.07), Vector3(-3.95, 1.725, z), trim)
	# Keep the exterior opening solid to the player while preserving light.
	var glass := material(Color(0.61, 0.75, 0.8, 0.12))
	glass.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	box("WindowGlass", Vector3(0.04, 1.75, 3), Vector3(-4, 1.725, -0.4), glass)
	box("Rug", Vector3(3.2, 0.012, 2.8), Vector3(0.2, 0.01, 0.2), material(Color(0.22, 0.3, 0.28)), false)
	model("Desk", "metal_office_desk", Vector3(0, 0, -2.2))
	model("Chair", "dining_chair_02", Vector3(0.15, 0, -0.85), PI + 0.2, true)
	model("Bookshelf", "wooden_bookshelf_worn", Vector3(2.9, 0, -3.9), 0, true)
	model("Plant", "potted_plant_02", Vector3(-2.8, 0, -2.8), 0, true)
	model("Lamp", "desk_lamp_arm_01", Vector3(-0.65, 0.8, -2.15))
	model("Books", "book_encyclopedia_set_01", Vector3(0.62, 0.8, -2.2))
	var monitor := Node3D.new()
	monitor.name = "MonitorSlot"
	get_node("Desk").add_child(monitor)
	# The screen faces the chair, along the desk's local +Z.
	monitor.position = Vector3(0, 0.8, 0)
	var dark := material(Color(0.025, 0.03, 0.035))
	box("Stand", Vector3(0.34, 0.025, 0.22), Vector3(0, 0.015, 0), dark, false, monitor)
	box("Stem", Vector3(0.055, 0.22, 0.05), Vector3(0, 0.13, 0), dark, false, monitor)
	box("Housing", Vector3(0.94, 0.55, 0.045), Vector3(0, 0.4, 0), dark, false, monitor)
	var screen_material := material(Color(0.04, 0.075, 0.09))
	screen_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	box("ScreenBackground", Vector3(0.885, 0.495, 0.006), Vector3(0, 0.4, 0.026), screen_material, false, monitor)
	var screen := MeshInstance3D.new()
	screen.name = "Screen"
	screen.mesh = QuadMesh.new()
	(screen.mesh as QuadMesh).size = Vector2(0.885, 0.495)
	screen.material_override = preload("res://scripts/screen_material.gd").create()
	screen.visible = false
	monitor.add_child(screen)
	screen.position = Vector3(0, 0.4, 0.031)
	var text := Label3D.new()
	text.text = "Starting terminal…"
	text.font_size = 32
	text.pixel_size = 0.001
	text.position = Vector3(0, 0.4, 0.031)
	monitor.add_child(text)
	var environment := WorldEnvironment.new()
	var env := Environment.new()
	env.background_mode = Environment.BG_SKY
	var sky := Sky.new()
	var sky_material := ProceduralSkyMaterial.new()
	sky_material.sky_top_color = Color(0.3, 0.49, 0.66)
	sky_material.sky_horizon_color = Color(0.8, 0.85, 0.9)
	sky.sky_material = sky_material
	env.sky = sky
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.78, 0.83, 0.91)
	env.ambient_light_energy = 0.3
	# Screen material inverts this fixed curve; its GPU color test guards changes.
	env.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	env.tonemap_exposure = 1.0
	env.tonemap_white = 1.0
	environment.environment = env
	add_child(environment)
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-35, -65, 0)
	sun.light_color = Color(1, 0.88, 0.7)
	sun.light_energy = 0.65
	sun.shadow_enabled = true
	add_child(sun)
	var details := preload("res://scripts/study_details.gd").new()
	details.name = "Details"
	add_child(details)
	var lamp := OmniLight3D.new()
	lamp.position = Vector3(-0.65, 1.3, -2.15)
	lamp.light_color = Color(1, 0.65, 0.3)
	lamp.light_energy = 0.3
	lamp.omni_range = 1.7
	add_child(lamp)
	configure_shadow_distances()
	desktop_runtime.initialize(get_node("Player"), screen, text, preload("res://scripts/hallway.gd").bounded_center)
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--capture="):
			capture_views(arg.trim_prefix("--capture="))

func configure_shadow_distances() -> void:
	# A distant small room's lamps should not redraw their shadow maps while
	# exploring the foyer. Keep nearby shadows, and fade rather than pop them.
	for light in find_children("*", "Light3D", true, false):
		if not light.shadow_enabled or light is DirectionalLight3D: continue
		var radius: float = light.omni_range if light is OmniLight3D else light.spot_range
		light.distance_fade_enabled = true
		light.distance_fade_shadow = maxf(15.0, radius * 2.0)
		light.distance_fade_begin = maxf(20.0, radius * 3.0)
		light.distance_fade_length = 5.0

func capture_views(directory: String) -> void:
	DirAccess.make_dir_recursive_absolute(directory)
	var player := get_node("Player") as CharacterBody3D
	player.set_physics_process(false)
	var camera := player.get_node("Camera3D") as Camera3D
	var positions := [Vector3(2.4, 1.65, 3.1), Vector3(0.5, 1.65, 0.2), Vector3(-2.8, 1.65, -3.4)]
	var targets := [Vector3(0, 1, -2), Vector3(0, 1.1, -2.2), Vector3(0.4, 1.1, -0.4)]
	for i in range(positions.size()):
		camera.global_position = positions[i]
		camera.look_at(targets[i])
		for frame in range(8):
			await get_tree().process_frame
		await RenderingServer.frame_post_draw
		var image := get_viewport().get_texture().get_image()
		var path := directory.path_join("study-%d.png" % i)
		assert(image.save_png(path) == OK, "Could not save " + path)
		print("CAPTURE ", path)
	await desktop_runtime.shutdown()
	get_tree().quit()
