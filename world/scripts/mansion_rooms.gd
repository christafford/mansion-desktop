## Six authored exploration spaces. Coordinates and IDs belong to the world,
## independent of application processes and live-window handles.
extends Node3D

const ROOMS := [
	{"id": "library", "title": "01  THE LONG LIBRARY", "subtitle": "Read • collect • connect", "origin": Vector3(1.4, 0, 7), "yaw": PI / 2, "width": 3.8, "depth": 12.8, "color": "40574b"},
	{"id": "atlas", "title": "02  THE ATLAS ROOM", "subtitle": "Explore • plan • discover", "origin": Vector3(-1.4, 0, 9), "yaw": -PI / 2, "width": 3.8, "depth": 12.8, "color": "657a76"},
	{"id": "garden", "title": "03  THE WINTER GARDEN", "subtitle": "A place for growing ideas", "origin": Vector3(1.4, 0, 11), "yaw": PI / 2, "width": 3.8, "depth": 12.8, "color": "afbda0"},
	{"id": "gallery", "title": "04  THE CABINET GALLERY", "subtitle": "Look • compare • imagine", "origin": Vector3(-1.4, 0, 13), "yaw": -PI / 2, "width": 3.8, "depth": 12.8, "color": "74444c"},
	{"id": "workshop", "title": "05  THE INVENTOR'S ROOM", "subtitle": "Build • test • rethink", "origin": Vector3(1.4, 0, 15), "yaw": PI / 2, "width": 3.8, "depth": 6.4, "color": "4e6171"},
	{"id": "observatory", "title": "06  THE OBSERVATORY", "subtitle": "Make room for the big picture", "origin": Vector3(0, 0, 18.5), "yaw": 0.0, "width": 9.0, "depth": 8.0, "color": "283c59"}
]
static var volumes: Array[AABB] = []
var study: Node3D
var hall: Node3D
var wood: StandardMaterial3D
var brass: StandardMaterial3D
var ivory: StandardMaterial3D
var iron: StandardMaterial3D
var rooms: Array[Node3D] = []

static func placement_volumes() -> Array[AABB]:
	if not volumes.is_empty(): return volumes
	for spec in ROOMS:
		var transform := Transform3D(Basis(Vector3.UP, spec.yaw), spec.origin)
		# The threshold volume overlaps both the hall and room. Its width follows
		# the clear opening, not the decorative architrave.
		volumes.append(transform * AABB(Vector3(-0.87, 0.08, -1.3), Vector3(1.74, 2.38, 2.8)))
		volumes.append(transform * AABB(Vector3(-spec.width / 2 + 0.14, 0.08, 0.14), Vector3(spec.width - 0.28, 3.0, spec.depth - 0.28)))
	return volumes

func _ready() -> void:
	hall = get_parent()
	study = hall.study
	wood = hall.wood
	brass = hall.brass
	ivory = study.material(Color("d8ceb5"))
	iron = study.material(Color("25322f"))
	iron.metallic = 0.6
	iron.roughness = 0.4
	for spec in ROOMS:
		var room := Node3D.new()
		room.name = spec.id
		room.set_meta("room_id", "mansion." + spec.id)
		room.set_meta("depth", spec.depth)
		room.add_to_group("mansion_rooms")
		add_child(room)
		room.position = spec.origin
		room.rotation.y = spec.yaw
		rooms.append(room)
		shell(room, spec)
		match spec.id:
			"library": library(room)
			"atlas": atlas(room)
			"garden": garden(room)
			"gallery": gallery(room)
			"workshop": workshop(room)
			"observatory": observatory(room)

	# Children such as books/lamps are part of their furniture assembly. Measure
	# after authoring completes so drag bounds cover the whole visible object.
	for body in get_tree().get_nodes_in_group("room_furniture"):
		body.measure_visuals()

func movable(parent: Node3D, label: String, pos: Vector3, weight := 12.0) -> RigidBody3D:
	var body := preload("res://scripts/pushable_furniture.gd").new()
	body.name = label
	body.kind = "assembly"
	body.assembly_mass = weight
	body.add_to_group("room_furniture")
	parent.add_child(body)
	body.position = pos
	return body

func part(parent: Node3D, label: String, size: Vector3, pos: Vector3, mat: Material, solid := false) -> MeshInstance3D:
	if solid and parent is RigidBody3D:
		parent.shape(size, pos)
		return study.box(label, size, pos, mat, false, parent)
	return study.box(label, size, pos, mat, solid, parent)

func mesh_part(parent: Node3D, label: String, mesh: Mesh, pos: Vector3, mat: Material) -> MeshInstance3D:
	var result := MeshInstance3D.new()
	result.name = label
	result.mesh = mesh
	result.material_override = mat
	parent.add_child(result)
	result.position = pos
	return result

func cylinder(parent: Node3D, label: String, pos: Vector3, radius: float, height: float, mat: Material) -> MeshInstance3D:
	var mesh := CylinderMesh.new()
	mesh.top_radius = radius
	mesh.bottom_radius = radius
	mesh.height = height
	mesh.radial_segments = 32
	return mesh_part(parent, label, mesh, pos, mat)

func ring(parent: Node3D, pos: Vector3, radius: float, thickness: float, mat: Material, rotation_value := Vector3.ZERO) -> MeshInstance3D:
	var mesh := TorusMesh.new()
	mesh.inner_radius = radius - thickness
	mesh.outer_radius = radius + thickness
	mesh.rings = 48
	mesh.ring_segments = 8
	var result := mesh_part(parent, "Ring", mesh, pos, mat)
	result.rotation = rotation_value
	return result

func text(parent: Node3D, value: String, pos: Vector3, size := 32, scale_value := 0.002) -> Label3D:
	var label := Label3D.new()
	label.text = value
	label.font_size = size
	label.pixel_size = scale_value
	label.modulate = Color("e7d7ac")
	label.outline_size = 0
	label.no_depth_test = false
	parent.add_child(label)
	label.position = pos
	return label

func prop(parent: Node3D, asset: String, pos: Vector3, yaw := 0.0, scale_value := 1.0) -> Node3D:
	var node: Node3D = load("res://assets/" + asset + ".gltf").instantiate()
	if asset in ["dining_chair_02", "potted_plant_02", "wooden_bookshelf_worn"]:
		var body := preload("res://scripts/pushable_furniture.gd").new()
		body.name = asset
		body.kind = asset
		body.model_scale = scale_value
		body.add_to_group("room_furniture")
		parent.add_child(body)
		body.position = pos
		body.rotation.y = yaw
		body.add_child(node)
		node.scale *= scale_value
		return body
	parent.add_child(node)
	node.position = pos
	node.rotation.y = yaw
	node.scale *= scale_value
	# Books and table accessories are carried as part of the assembled object.
	# Never nest a StaticBody inside a moving rigid body.
	if not parent is RigidBody3D:
		for mesh in node.find_children("*", "MeshInstance3D", true, false): mesh.create_trimesh_collision()
	return node

func shell(room: Node3D, spec: Dictionary) -> void:
	var w: float = spec.width
	var d: float = spec.depth
	var wall = study.material(Color(spec.color), "res://assets/textures/beige_wall_001_4k_jpg.jpg", 3)
	var floor_mat = study.material(Color("ad9271"), "res://assets/textures/walnut_veneer_4k_jpg.jpg", 4)
	if spec.id in ["garden", "gallery", "observatory"]:
		floor_mat = ShaderMaterial.new()
		floor_mat.shader = preload("res://scripts/mansion_stone.gdshader")
		floor_mat.set_shader_parameter("checker", spec.id == "garden")
		floor_mat.set_shader_parameter("base_color", Color("aea88c") if spec.id == "garden" else Color("393d45"))
	part(room, "Floor", Vector3(w, 0.16, d + 0.2), Vector3(0, -0.08, d / 2), floor_mat, true)
	for x in [-w / 2, w / 2]:
		part(room, "SideWall", Vector3(0.16, 3.3, d), Vector3(x, 1.65, d / 2), wall, true)
		part(room, "Skirting", Vector3(0.09, 0.19, d), Vector3(x - signf(x) * 0.1, 0.095, d / 2), wood)
		part(room, "PictureRail", Vector3(0.06, 0.045, d), Vector3(x - signf(x) * 0.1, 2.72, d / 2), brass)
		part(room, "Cornice", Vector3(0.16, 0.14, d), Vector3(x - signf(x) * 0.05, 3.14, d / 2), ivory)
	part(room, "BackWall", Vector3(w, 3.3, 0.16), Vector3(0, 1.65, d), wall, true)
	part(room, "BackSkirting", Vector3(w, 0.19, 0.09), Vector3(0, 0.095, d - 0.1), wood)
	# The hallway owns the actual entrance wall and collision.
	for x in [-1, 1]:
		var width := (w - 1.8) / 2
		part(room, "EntryReturn", Vector3(width, 3.3, 0.12), Vector3(x * (0.9 + width / 2), 1.65, 0), wall, true)
		part(room, "InnerDoorFrame", Vector3(0.1, 2.53, 0.08), Vector3(x * 0.95, 1.265, 0.13), wood)
	part(room, "InnerLintel", Vector3(2, 0.13, 0.08), Vector3(0, 2.55, 0.13), wood)
	part(room, "Header", Vector3(1.8, 0.8, 0.12), Vector3(0, 2.9, 0), wall, true)
	part(room, "Threshold", Vector3(1.8, 0.012, 0.24), Vector3(0, 0.006, 0), brass)
	if spec.id == "garden":
		var glass = study.material(Color(0.58, 0.75, 0.74, 0.18))
		glass.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		part(room, "GlassRoof", Vector3(w, 0.06, d), Vector3(0, 3.3, d / 2), glass, true).cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		for z in range(floori(d) + 1): part(room, "RoofRafter", Vector3(w, 0.09, 0.055), Vector3(0, 3.26, z), iron)
		for x in [-1.2, 0.0, 1.2]: part(room, "RoofRib", Vector3(0.06, 0.12, d), Vector3(x, 3.23, d / 2), iron)
	else:
		part(room, "Ceiling", Vector3(w, 0.12, d), Vector3(0, 3.36, d / 2), study.material(Color("202e44") if spec.id == "observatory" else Color("b9b7a5")), true)
		for step in range(ceili(d / 2.2)):
			var z := 1.1 + step * 2.2
			part(room, "Coffer", Vector3(w, 0.11, 0.09), Vector3(0, 3.23, z), wood)
	# Named plaques face into the hall; no extra key or modal UI to enter.
	var sign_node := Node3D.new()
	room.add_child(sign_node)
	sign_node.position = Vector3(0, 2.84, -0.19)
	sign_node.rotation.y = PI
	part(sign_node, "Nameplate", Vector3(1.7, 0.28, 0.025), Vector3.ZERO, iron)
	text(sign_node, spec.title, Vector3(0, 0, 0.02), 30, 0.0028)
	# An inscription over the inner doorway also helps orient the return.
	text(room, spec.subtitle, Vector3(0, 2.88, 0.14), 30, 0.0022)
	if spec.id != "garden":
		var light_color := Color("ffe0ad")
		if spec.id in ["atlas", "workshop"]: light_color = Color("d1e3ed")
		elif spec.id == "observatory": light_color = Color("b9d4ff")
		for step in range(ceili(d / 3.2)):
			pendant(room, Vector3(0, 2.87, (step + 0.5) * d / ceili(d / 3.2)), light_color)
	if spec.id in ["library", "gallery"]:
		for side in [-1, 1]:
			for step in range(floori(d / 1.5)):
				var z := 1.0 + step * 1.5
				var x: float = side * (w / 2 - 0.105)
				for offset in [-0.58, 0.58]:
					part(room, "PanelStile", Vector3(0.024, 0.68, 0.022), Vector3(x, 0.57, z + offset), wood)
				for y in [0.23, 0.91]:
					part(room, "PanelRail", Vector3(0.024, 0.022, 1.18), Vector3(x, y, z), wood)
			part(room, "Dado", Vector3(0.05, 0.045, d), Vector3(side * (w / 2 - 0.11), 1.0, d / 2), wood)

func pendant(room: Node3D, pos: Vector3, color: Color) -> void:
	cylinder(room, "PendantStem", pos + Vector3(0, 0.21, 0), 0.014, 0.42, brass)
	var shade = hall.lathe("PendantShade", [Vector2(0.12, 0.04), Vector2(0.17, 0.03), Vector2(0.25, -0.03), Vector2(0.31, -0.1), Vector2(0.31, -0.12)], brass, room)
	shade.position = pos
	var glow = study.material(color)
	glow.emission_enabled = true
	glow.emission = color
	glow.emission_energy_multiplier = 1.1
	cylinder(room, "Diffuser", pos + Vector3(0, -0.11, 0), 0.28, 0.012, glow)
	var light := OmniLight3D.new()
	room.add_child(light)
	light.position = pos + Vector3(0, -0.2, 0)
	light.light_color = color
	light.light_energy = 0.42
	light.omni_range = 5.0
	light.omni_attenuation = 1.1
	light.shadow_enabled = true

func table(room: Node3D, pos: Vector3, size: Vector2, mat: Material = wood) -> Node3D:
	var node := movable(room, "Worktable", pos, 18.0)
	node.add_to_group("room_work_surfaces")
	part(node, "Top", Vector3(size.x, 0.09, size.y), Vector3(0, 0.84, 0), mat, true)
	part(node, "Apron", Vector3(size.x - 0.08, 0.18, size.y - 0.08), Vector3(0, 0.72, 0), wood)
	for x in [-1, 1]:
		for z in [-1, 1]:
			var leg = hall.lathe("TurnedLeg", [Vector2(0.035, 0), Vector2(0.05, 0.06), Vector2(0.029, 0.15), Vector2(0.04, 0.56), Vector2(0.062, 0.64), Vector2(0.045, 0.8)], mat, node)
			leg.position = Vector3(x * (size.x / 2 - 0.11), 0, z * (size.y / 2 - 0.11))
			node.shape(Vector3(0.1, 0.8, 0.1), leg.position + Vector3(0, 0.4, 0))
	return node

func artwork(room: Node3D, pos: Vector3, yaw: float, texture: String, size: Vector2) -> void:
	var node := Node3D.new()
	room.add_child(node)
	node.position = pos
	node.rotation.y = yaw
	part(node, "Frame", Vector3(size.x + 0.09, size.y + 0.09, 0.06), Vector3.ZERO, wood)
	part(node, "Mat", Vector3(size.x + 0.025, size.y + 0.025, 0.012), Vector3(0, 0, 0.039), ivory)
	var quad := QuadMesh.new()
	quad.size = size
	mesh_part(node, "Artwork", quad, Vector3(0, 0, 0.05), study.material(Color.WHITE, texture))

func library(room: Node3D) -> void:
	preload("res://scripts/antique_library.gd").new().build(self, room)

func atlas(room: Node3D) -> void:
	artwork(room, Vector3(0, 1.98, 12.68), PI, "res://art/atlas.svg", Vector2(2.7, 1.45))
	var desk := table(room, Vector3(0, 0, 11.5), Vector2(2.65, 1.25))
	# Open central table for application placement; a small chart sits flush.
	var chart := part(desk, "Chart", Vector3(0.9, 0.004, 0.6), Vector3(-0.7, 0.891, 0), study.material(Color("c8ba8e")))
	chart.rotation.y = 0.15
	var cabinet := movable(room, "MapCabinet", Vector3(1.5, 0, 5.4), 20.0)
	part(cabinet, "FlatFileCase", Vector3(0.51, 1.19, 1.67), Vector3(0, 0.595, 0), hall.dark, true)
	for i in range(8):
		part(cabinet, "MapDrawer", Vector3(0.055, 0.12, 1.6), Vector3(-0.28, 0.1 + i * 0.135, 0), wood)
		part(cabinet, "DrawerPull", Vector3(0.035, 0.025, 0.17), Vector3(-0.33, 0.12 + i * 0.135, 0), brass)
	armillary(room, Vector3(-1.3, 0, 3.7), 0.46)
	artwork(room, Vector3(-1.79, 1.9, 8.3), PI / 2, "res://art/atlas.svg", Vector2(1.8, 1.2))

func garden(room: Node3D) -> void:
	for spec in [[-1.2, 1.3, 1.0], [1.2, 4.2, 1.2], [-1.22, 7.4, 1.35], [1.12, 11.3, 1.0], [1.2, 7.5, 1.15], [-1.2, 10.7, 1.0]]:
		prop(room, "potted_plant_02", Vector3(spec[0], 0, spec[1]), spec[1], spec[2])
	table(room, Vector3(0.15, 0, 11.5), Vector2(1.8, 0.95), ivory)
	artwork(room, Vector3(1.79, 1.9, 9), -PI / 2, "res://art/ginkgo.svg", Vector2(0.7, 1.0))
	for z in [1.0, 3.0, 5.0, 7.0, 9.0, 11.0]:
		for x in [-1.79, 1.79]:
			part(room, "Pilaster", Vector3(0.08, 2.9, 0.1), Vector3(x, 1.45, z), ivory)
	# A shallow fountain bowl, raised rim and subtle concentric water rings.
	var bowl = hall.lathe("Fountain", [Vector2(0, 0), Vector2(0.28, 0), Vector2(0.3, 0.1), Vector2(0.13, 0.25), Vector2(0.16, 0.65), Vector2(0.47, 0.78), Vector2(0.5, 0.86), Vector2(0.46, 0.89), Vector2(0.41, 0.79), Vector2(0, 0.76)], ivory, room)
	bowl.position = Vector3(-0.75, 0, 2.85)
	bowl.create_trimesh_collision()
	var water = study.material(Color("467879"))
	water.metallic = 0.5
	water.roughness = 0.2
	cylinder(room, "Water", Vector3(-0.75, 0.8, 2.85), 0.415, 0.005, water)
	for radius in [0.1, 0.21, 0.33]: ring(room, Vector3(-0.75, 0.805, 2.85), radius, 0.002, brass)

func gallery(room: Node3D) -> void:
	for z in [3.4, 8.8]:
		var x := -1.2 if z < 5 else 1.2
		var plinth := movable(room, "DisplayStand", Vector3(x, 0, z), 16.0)
		part(plinth, "PlinthFoot", Vector3(0.77, 0.09, 0.77), Vector3(0, 0.045, 0), ivory, true)
		part(plinth, "Plinth", Vector3(0.62, 0.9, 0.62), Vector3(0, 0.54, 0), ivory, true)
		part(plinth, "PlinthCap", Vector3(0.72, 0.07, 0.72), Vector3(0, 1.025, 0), ivory, true)
		var sculpture := Node3D.new()
		plinth.add_child(sculpture)
		sculpture.position = Vector3(0, 1.53, 0)
		plinth.shape(Vector3(0.76, 0.88, 0.76), Vector3(0, 1.52, 0))
		if z < 5:
			for i in range(3): ring(sculpture, Vector3.ZERO, 0.39, 0.035, brass, Vector3(PI / 2, i * PI / 3, 0))
		else:
			var mat = study.material(Color("b77c58"))
			for i in range(9):
				var disc := cylinder(sculpture, "SpiralStudy", Vector3(sin(i * 0.5) * 0.12, -0.4 + i * 0.095, cos(i * 0.5) * 0.12), 0.25 - i * 0.012, 0.08, mat)
				disc.rotation.z = i * 0.04
	artwork(room, Vector3(1.79, 1.95, 4.0), -PI / 2, "res://art/geometry.svg", Vector2(1.0, 1.35))
	artwork(room, Vector3(-1.79, 1.95, 8.6), PI / 2, "res://art/geometry.svg", Vector2(1.0, 1.35))
	table(room, Vector3(0, 0, 11.9), Vector2(2.45, 0.85), ivory)

func workshop(room: Node3D) -> void:
	var desk := table(room, Vector3(0, 0, 5.45), Vector2(2.95, 1.2), iron)
	prop(desk, "desk_lamp_arm_01", Vector3(1.1, 0.89, 0.2))
	artwork(room, Vector3(0, 2.03, 6.27), PI, "res://art/mechanism.svg", Vector2(2.6, 1.35))
	part(room, "ToolBoard", Vector3(0.08, 1.3, 2.1), Vector3(-1.77, 1.78, 3), wood)
	for z in [2.3, 2.65, 3.0, 3.35, 3.7]:
		part(room, "ToolHandle", Vector3(0.06, 0.35, 0.055), Vector3(-1.69, 1.73, z), iron)
		part(room, "ToolHead", Vector3(0.065, 0.07, 0.19), Vector3(-1.68, 1.94, z), brass)
	for y in [0.3, 0.7, 1.1]:
		part(room, "PartsShelf", Vector3(0.48, 0.06, 1.8), Vector3(1.5, y, 2.7), wood, true)
		for z in [2.1, 2.5, 2.9, 3.3]:
			var bin := movable(room, "PartsBin", Vector3(1.5, y + 0.031, z), 2.0)
			part(bin, "BinBase", Vector3(0.35, 0.025, 0.3), Vector3(0, 0.013, 0), iron, true)
			for edge in [-1, 1]:
				part(bin, "BinSide", Vector3(0.35, 0.18, 0.018), Vector3(0, 0.1, edge * 0.145), iron, true)
				part(bin, "BinEnd", Vector3(0.018, 0.18, 0.3), Vector3(edge * 0.17, 0.1, 0), iron, true)
			part(bin, "BinLabel", Vector3(0.005, 0.04, 0.12), Vector3(-0.19, 0.13, 0), ivory)

	prop(room, "dining_chair_02", Vector3(0.7, 0, 4.15), 0)

func armillary(room: Node3D, pos: Vector3, radius: float) -> void:
	var instrument := movable(room, "Armillary", pos, 10.0)
	instrument.shape(Vector3(0.64, 0.84, 0.64), Vector3(0, 0.42, 0))
	instrument.shape(Vector3(radius * 2, radius * 2, radius * 2), Vector3(0, 0.83 + radius, 0))
	var stand = hall.lathe("InstrumentPedestal", [Vector2(0, 0), Vector2(0.32, 0), Vector2(0.34, 0.07), Vector2(0.12, 0.13), Vector2(0.09, 0.73), Vector2(0.2, 0.82), Vector2(0, 0.85)], wood, instrument)
	stand.position = Vector3.ZERO
	var center := Vector3(0, 0.83 + radius, 0)
	ring(instrument, center, radius, 0.016, brass, Vector3(PI / 2, 0, 0))
	ring(instrument, center, radius * 0.94, 0.016, brass, Vector3(0, 0, 0.4))
	ring(instrument, center, radius * 0.88, 0.012, brass, Vector3(PI / 2, 0.9, 0.5))
	var globe := SphereMesh.new()
	globe.radius = radius * 0.28
	globe.height = radius * 0.56
	mesh_part(instrument, "Globe", globe, center, study.material(Color("467b85")))
	for i in range(24):
		var angle := TAU * i / 24
		var tick := part(instrument, "MeridianMark", Vector3(0.013, 0.034, 0.02), center + Vector3(sin(angle), cos(angle), 0) * radius, ivory)
		tick.rotation.z = -angle

func observatory(room: Node3D) -> void:
	armillary(room, Vector3(0, 0, 4.9), 0.95)
	# Inlaid meridian circles lead around the central instrument, not through it.
	for radius in [1.7, 1.76, 2.5]: ring(room, Vector3(0, 0.015, 4.9), radius, 0.012, brass)
	for side in [-1, 1]:
		table(room, Vector3(side * 2.9, 0, 5.7), Vector2(2.0, 1.2))
		artwork(room, Vector3(side * 2.8, 2.03, 7.86), PI, "res://art/stars.svg", Vector2(2.05, 1.55))
		prop(room, "dining_chair_02", Vector3(side * 2.9, 0, 4.5), 0)
		pendant(room, Vector3(side * 2.8, 2.87, 5.7), Color("ffdeaa"))
	# A star-chart ceiling is authored geometry, not a simulated exterior sky.
	var star_mat = study.material(Color("ccddf3"))
	star_mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	var rng := RandomNumberGenerator.new()
	rng.seed = 6106
	for i in range(90):
		cylinder(room, "CeilingStar", Vector3(rng.randf_range(-4.1, 4.1), 3.285, rng.randf_range(0.4, 7.6)), rng.randf_range(0.007, 0.018), 0.006, star_mat)
	for radius in [1.0, 1.8, 2.6]: ring(room, Vector3(0, 3.27, 4.5), radius, 0.008, brass)
