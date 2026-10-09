## The study's connected north gallery. Authored joinery and oil-lamp meshes.
extends Node3D

const START := 4.5
const END := 18.5
# Inset placement volumes overlap through the opening, allowing carried objects
# to straddle the threshold without admitting placements outside the building.
const PLACEMENT := [
	AABB(Vector3(-3.86, 0.08, -4.36), Vector3(7.72, 3.07, 8.72)),
	AABB(Vector3(-0.81, 0.08, 2.5), Vector3(1.62, 2.38, 4.0)),
	AABB(Vector3(-1.27, 0.08, 4.6), Vector3(2.54, 2.87, 13.76))]
var study: Node3D
var wood: StandardMaterial3D
var brass: StandardMaterial3D
var dark: StandardMaterial3D
var lights: Array[OmniLight3D] = []
var flames: Array[MeshInstance3D] = []
var elapsed := 0.0

static func bounded_center(center: Vector3, extents: Vector3) -> Vector3:
	var result := center
	var nearest := INF
	for volume in PLACEMENT + preload("res://scripts/mansion_rooms.gd").placement_volumes():
		if volume.size.x < extents.x * 2 or volume.size.y < extents.y * 2 or volume.size.z < extents.z * 2: continue
		var candidate := center.clamp(volume.position + extents, volume.end - extents)
		var distance := candidate.distance_squared_to(center)
		if distance < nearest:
			nearest = distance
			result = candidate
	return result

func _ready() -> void:
	study = get_parent()
	wood = study.material(Color("9c7855"), "res://assets/textures/walnut_veneer_4k_jpg.jpg")
	wood.roughness = 0.55
	brass = study.material(Color("aa854c"))
	brass.metallic = 0.72
	brass.roughness = 0.3
	dark = study.material(Color("231c18"))
	build_shell()
	entry_frame()
	# Facing away from the study (+Z), left is +X and right is -X.
	for z in [11.0, 15.0]: door(Vector3(1.28, 0, z), -PI / 2, "LeftDoor%d" % z)
	for z in [9.0]: door(Vector3(-1.28, 0, z), PI / 2, "RightDoor%d" % z)
	door(Vector3(0, 0, END - 0.09), PI, "EndDoor")
	var rooms := preload("res://scripts/mansion_rooms.gd").new()
	rooms.name = "Rooms"
	add_child(rooms)
	for z in [9.0, 13.0, 17.0]: oil_lamp(Vector3(1.29, 1.42, z), -PI / 2)
	for z in [7.0, 11.0, 15.0]: oil_lamp(Vector3(-1.29, 1.42, z), PI / 2)

func part(label: String, size: Vector3, pos: Vector3, mat: Material, parent: Node3D = self, solid := false) -> MeshInstance3D:
	return study.box(label, size, pos, mat, solid, parent)

func build_shell() -> void:
	var plaster = study.material(Color("b6a48c"), "res://assets/textures/beige_wall_001_4k_jpg.jpg", 5)
	part("Floor", Vector3(2.8, 0.16, 14.08), Vector3(0, -0.08, 11.46), dark, self, true)
	part("Ceiling", Vector3(2.96, 0.12, 14.08), Vector3(0, 3.06, 11.46), study.material(Color("aa9b85")), self, true)
	# Break the actual walls, wainscot and rails around the remaining entrances.
	for x in [-1.4, 1.4]:
		var openings := [11.0, 15.0] if x > 0 else [9.0]
		var start := START
		for z in openings + [END + 0.9]:
			var finish: float = z - 0.9
			var length := finish - start
			var middle := (finish + start) / 2
			part("Wall", Vector3(0.16, 3, length), Vector3(x, 1.5, middle), plaster, self, true)
			part("Wainscot", Vector3(0.035, 0.94, length), Vector3(signf(x) * 1.305, 0.5, middle), wood)
			for y in [0.1, 1.0]:
				part("Moulding", Vector3(0.09, 0.075 if y != 0.1 else 0.18, length), Vector3(signf(x) * 1.28, y, middle), wood)
			start = z + 0.9
		for z in openings:
			part("DoorHeader", Vector3(0.16, 0.5, 1.8), Vector3(x, 2.75, z), plaster, self, true)
		part("Cornice", Vector3(0.09, 0.075, 14), Vector3(signf(x) * 1.28, 2.91, 11.5), wood)
	for x in [-1.15, 1.15]:
		part("EndWall", Vector3(0.5, 3, 0.16), Vector3(x, 1.5, END), plaster, self, true)
	part("EndHeader", Vector3(1.8, 0.5, 0.16), Vector3(0, 2.75, END), plaster, self, true)
	for z in [4.65, 8.5, 12.5, 16.5, 18.38]:
		part("CeilingCoffer", Vector3(2.65, 0.075, 0.1), Vector3(0, 2.945, z), wood)
	var carpet := ShaderMaterial.new()
	carpet.shader = preload("res://scripts/hall_carpet.gdshader")
	part("BurgundyCarpet", Vector3(2.52, 0.016, 14.06), Vector3(0, 0.009, 11.47), carpet)
	# Dark bound edges and double woven gold lines, inset from each side.
	for x in [-1.21, 1.21, -1.12, 1.12]:
		part("CarpetBorder", Vector3(0.015, 0.001, 13.95), Vector3(x, 0.0176, 11.5), study.material(Color("80603e")))
	part("Threshold", Vector3(1.7, 0.012, 0.24), Vector3(0, 0.006, START), brass)

func entry_frame() -> void:
	for z in [START - 0.12, START + 0.12]:
		for x in [-0.91, 0.91]:
			part("EntryArchitrave", Vector3(0.12, 2.58, 0.07), Vector3(x, 1.29, z), wood)
		part("EntryLintel", Vector3(1.94, 0.14, 0.075), Vector3(0, 2.57, z), wood)
	for x in [-0.865, 0.865]:
		part("EntryJamb", Vector3(0.03, 2.5, 0.24), Vector3(x, 1.25, START), wood)
	part("EntrySoffit", Vector3(1.7, 0.03, 0.24), Vector3(0, 2.515, START), wood)

func door(pos: Vector3, yaw: float, label: String) -> void:
	var node := Node3D.new()
	node.name = label
	node.add_to_group("hallway_doors")
	add_child(node)
	node.position = pos
	node.rotation.y = yaw
	# The leaf swings into its room once approached; it never closes on a carry.
	for x in [-0.94, 0.94]:
		part("Architrave", Vector3(0.13, 2.56, 0.1), Vector3(x, 1.28, 0.055), wood, node)
		part("FrameBead", Vector3(0.022, 2.47, 0.02), Vector3(x - signf(x) * 0.07, 1.25, 0.11), brass, node)
	part("Lintel", Vector3(2.01, 0.14, 0.12), Vector3(0, 2.53, 0.055), wood, node)
	part("Cornice", Vector3(2.1, 0.05, 0.16), Vector3(0, 2.625, 0.06), wood, node)
	var hinge := preload("res://scripts/exploration_door.gd").new()
	hinge.name = "Hinge"
	node.add_child(hinge)
	hinge.position = Vector3(0.85, 0, 0)
	var leaf := Node3D.new()
	leaf.name = "DoorLeaf"
	hinge.add_child(leaf)
	leaf.position.x = -0.85
	part("Leaf", Vector3(1.7, 2.46, 0.06), Vector3(0, 1.25, 0), wood, leaf, true)
	for face in [-1, 1]:
		for x in [-0.42, 0.42]:
			for spec in [[0.56, 0.7], [1.64, 1.15]]:
				var y: float = spec[0]
				var h: float = spec[1]
				part("PanelRecess", Vector3(0.63, h, 0.012), Vector3(x, y, face * 0.036), dark, leaf)
				part("RaisedPanel", Vector3(0.57, h - 0.06, 0.016), Vector3(x, y, face * 0.043), wood, leaf)
				for side in [-1, 1]:
					part("PanelBead", Vector3(0.018, h, 0.025), Vector3(x + side * 0.315, y, face * 0.048), wood, leaf)
		part("LockPlate", Vector3(0.085, 0.23, 0.015), Vector3(-0.7, 1.02, face * 0.046), brass, leaf)
		var knob := lathe("BrassKnob", [Vector2(0.012, 0), Vector2(0.032, 0.018), Vector2(0.034, 0.042), Vector2(0.012, 0.06), Vector2(0, 0.062)], brass, leaf)
		knob.position = Vector3(-0.7, 1.08, face * 0.05)
		knob.rotation.x = face * PI / 2
	for y in [0.36, 2.03]: part("HingeBarrel", Vector3(0.025, 0.13, 0.03), Vector3(0.85, y, 0.035), brass, leaf)

func lathe(label: String, profile: Array, mat: Material, parent: Node3D) -> MeshInstance3D:
	# Revolved profiles give the reservoir, chimney and hardware curved silhouettes.
	var vertices := PackedVector3Array()
	var normals := PackedVector3Array()
	var indices := PackedInt32Array()
	const SEGMENTS := 32
	for j in range(profile.size()):
		var tangent: Vector2 = profile[mini(j + 1, profile.size() - 1)] - profile[maxi(0, j - 1)]
		var normal := Vector2(tangent.y, -tangent.x).normalized()
		for i in range(SEGMENTS + 1):
			var angle := TAU * i / SEGMENTS
			vertices.append(Vector3(cos(angle) * profile[j].x, profile[j].y, sin(angle) * profile[j].x))
			normals.append(Vector3(cos(angle) * normal.x, normal.y, sin(angle) * normal.x))
	for j in range(profile.size() - 1):
		for i in range(SEGMENTS):
			var a := j * (SEGMENTS + 1) + i
			indices.append_array(PackedInt32Array([a, a + 1, a + SEGMENTS + 1, a + 1, a + SEGMENTS + 2, a + SEGMENTS + 1]))
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = vertices
	arrays[Mesh.ARRAY_NORMAL] = normals
	arrays[Mesh.ARRAY_INDEX] = indices
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	var result := MeshInstance3D.new()
	result.name = label
	result.mesh = mesh
	result.material_override = mat
	parent.add_child(result)
	return result

func oil_lamp(pos: Vector3, yaw: float) -> void:
	var lamp := Node3D.new()
	lamp.name = "OilLamp%d" % lights.size()
	add_child(lamp)
	lamp.position = pos
	lamp.rotation.y = yaw
	var plate := lathe("OvalBackplate", [Vector2(0, 0), Vector2(0.072, 0), Vector2(0.085, 0.006), Vector2(0.077, 0.027), Vector2(0, 0.03)], brass, lamp)
	plate.position = Vector3(0, 0.13, 0.016)
	plate.rotation.x = PI / 2
	plate.scale = Vector3(1, 1, 2.6)
	for y in [-0.025, 0.285]:
		var screw := lathe("Fixing", [Vector2(0.006, 0), Vector2(0.009, 0.003), Vector2(0, 0.008)], dark, lamp)
		screw.position = Vector3(0, y, 0.05)
		screw.rotation.x = PI / 2
	part("Bracket", Vector3(0.035, 0.035, 0.3), Vector3(0, -0.065, 0.15), brass, lamp)
	var bowl := lathe("OilReservoir", [Vector2(0, -0.06), Vector2(0.055, -0.06), Vector2(0.108, -0.035), Vector2(0.12, 0.01), Vector2(0.10, 0.07), Vector2(0.055, 0.09), Vector2(0.04, 0.11)], brass, lamp)
	bowl.position.z = 0.28
	var glass = study.material(Color(0.78, 0.86, 0.79, 0.045))
	glass.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	glass.roughness = 0.22
	glass.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	glass.cull_mode = BaseMaterial3D.CULL_DISABLED
	var chimney := lathe("GlassChimney", [Vector2(0.065, 0.11), Vector2(0.089, 0.16), Vector2(0.078, 0.23), Vector2(0.055, 0.3), Vector2(0.049, 0.5), Vector2(0.059, 0.53)], glass, lamp)
	chimney.position.z = 0.28
	chimney.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	var flame_mat = study.material(Color("ffc76a"))
	flame_mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	flame_mat.emission_enabled = true
	flame_mat.emission = Color("ff9e36")
	flame_mat.emission_energy_multiplier = 2
	var flame := lathe("Flame", [Vector2(0, 0), Vector2(0.02, 0.018), Vector2(0.024, 0.045), Vector2(0.013, 0.078), Vector2(0, 0.12)], flame_mat, lamp)
	flame.position = Vector3(0, 0.115, 0.28)
	flame.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	flames.append(flame)
	var burner := lathe("Burner", [Vector2(0.038, 0.09), Vector2(0.042, 0.1), Vector2(0.028, 0.125), Vector2(0.01, 0.135)], dark, lamp)
	burner.position.z = 0.28
	var light := OmniLight3D.new()
	light.position = Vector3(0, 0.23, 0.33)
	light.light_color = Color("ffcc91")
	light.light_energy = 0.22
	light.omni_range = 4.8
	light.omni_attenuation = 1.3
	light.shadow_enabled = true
	light.shadow_bias = 0.04
	lamp.add_child(light)
	lights.append(light)

func _process(delta: float) -> void:
	elapsed += delta
	for i in range(lights.size()):
		var t := elapsed + i * 3.17
		var strength := 0.96 + 0.025 * sin(t * 4.3) + 0.015 * sin(t * 11.7)
		lights[i].light_energy = 0.22 * strength
		flames[i].scale = Vector3(1 + 0.06 * sin(t * 7), strength, 1)
