## Original furniture inspired by the owner's Bordeaux bookcase reference.
## Black frames, warm plank interiors and metal rods; no retailer images used.
extends RefCounted

var space: Node3D
var ebony: StandardMaterial3D
var oak: StandardMaterial3D
var worn: StandardMaterial3D
var book_mesh: ArrayMesh

func build(builder: Node3D, room: Node3D) -> void:
	space = builder
	ebony = space.study.material(Color("292822"), "res://assets/textures/walnut_veneer_4k_jpg.jpg")
	ebony.roughness = 0.48
	oak = space.study.material(Color("b79563"), "res://assets/textures/walnut_veneer_4k_jpg.jpg")
	oak.roughness = 0.6
	worn = space.study.material(Color("60523a"))
	# Two four-piece runs follow the outer wall, leaving the garden side open.
	# Leave 6cm between crowns so the shared
	# drag clearance permits pulling an individual bay out of the row.
	for i in range(4):
		bookcase(room, Vector3(1.55, 0, 2.4 + i * 1.12), -PI / 2, i)
		bookcase(room, Vector3(1.55, 0, 6.7 + i * 1.12), -PI / 2, i + 4)
	var desk = space.table(room, Vector3(0.3, 0, 11.7), Vector2(2.4, 1.15), ebony)
	var leather = space.study.material(Color("294437"))
	leather.roughness = 0.7
	space.part(desk, "LeatherWritingInset", Vector3(1.55, 0.008, 0.75), Vector3(0, 0.892, 0), leather)
	for x in [-0.78, 0.78]: space.part(desk, "InsetBorder", Vector3(0.008, 0.003, 0.76), Vector3(x, 0.898, 0), space.brass)
	for z in [-0.38, 0.38]: space.part(desk, "InsetBorder", Vector3(1.56, 0.003, 0.008), Vector3(0, 0.898, z), space.brass)
	banker_lamp(desk, Vector3(0.87, 0.889, 0.17))
	space.prop(room, "dining_chair_02", Vector3(0.3, 0, 10.6), 0)
	space.prop(room, "dining_chair_02", Vector3(0.65, 0, 3.6), -0.55)
	space.artwork(room, Vector3(1.79, 1.9, 1.2), -PI / 2, "res://art/fern.svg", Vector2(0.8, 1.2))
	space.artwork(room, Vector3(-0.8, 1.9, 12.68), PI, "res://art/atlas.svg", Vector2(1.6, 1.0))
	# A worn burgundy runner connects the entrance to the reading desk.
	var carpet := ShaderMaterial.new()
	carpet.shader = preload("res://scripts/hall_carpet.gdshader")
	space.part(room, "LibraryRunner", Vector3(1.5, 0.012, 9.3), Vector3(0, 0.01, 6), carpet)
	for x in [-0.68, 0.68]: space.part(room, "WovenBorder", Vector3(0.025, 0.002, 9.15), Vector3(x, 0.017, 6), worn)
	for z in [1.45, 10.55]: space.part(room, "WovenBorder", Vector3(1.38, 0.002, 0.025), Vector3(0, 0.017, z), worn)

func bookcase(room: Node3D, pos: Vector3, yaw: float, index: int) -> void:
	var body = space.movable(room, "AntiqueBookcase%d" % index, pos, 25.0)
	body.rotation.y = yaw
	body.add_to_group("antique_bookcases")
	space.part(body, "Plinth", Vector3(1.04, 0.105, 0.46), Vector3(0, 0.0525, 0), ebony, true)
	# Individual planks retain readable, vertically oriented photographic grain.
	space.part(body, "Back", Vector3(0.94, 2.2, 0.027), Vector3(0, 1.2, -0.19), oak, true)
	for i in range(7):
		var face := MeshInstance3D.new()
		var quad := QuadMesh.new()
		quad.size = Vector2(0.132, 2.18)
		face.mesh = quad
		var mat = oak.duplicate()
		mat.uv1_scale = Vector3(0.132 / 2, 2.18 / 2, 1)
		mat.uv1_offset = Vector3(fposmod(i * 0.173 + index * 0.09, 1), 0, 0)
		face.material_override = mat
		face.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		body.add_child(face)
		face.position = Vector3(-0.405 + i * 0.135, 1.195, -0.173)
	for x in [-0.485, 0.485]:
		space.part(body, "Side", Vector3(0.065, 2.2, 0.39), Vector3(x, 1.2, 0), ebony, true)
		space.part(body, "FrameBead", Vector3(0.016, 2.18, 0.015), Vector3(x, 1.2, 0.204), worn)
		space.cylinder(body, "IronUpright", Vector3(x * 0.87, 1.22, 0.228), 0.014, 2.19, space.iron)
		for y in [0.16, 0.53, 0.94, 1.35, 1.76, 2.17]:
			space.cylinder(body, "PipeCollar", Vector3(x * 0.87, y, 0.228), 0.022, 0.027, space.iron)
	for y in [0.115, 0.52, 0.93, 1.34, 1.75, 2.16]:
		space.part(body, "Shelf", Vector3(0.92, 0.032, 0.39), Vector3(0, y, 0), oak, true)
		space.part(body, "ShelfLip", Vector3(0.92, 0.016, 0.009), Vector3(0, y, 0.199), worn)
	for spec in [[2.27, 1.02, 0.07, 0.44], [2.325, 1.06, 0.04, 0.48], [2.353, 1.03, 0.016, 0.46]]:
		space.part(body, "CrownMoulding", Vector3(spec[1], spec[2], spec[3]), Vector3(0, spec[0], 0), ebony, true)
	for y in [0.14, 0.955, 1.775]:
		books(body, Vector3(-0.08, y, 0.02))
	# Small turned vessels balance the books; these belong to the shelf assembly.
	var vase = space.hall.lathe("ShelfVessel", [Vector2(0, 0), Vector2(0.055, 0), Vector2(0.082, 0.05), Vector2(0.073, 0.14), Vector2(0.027, 0.2), Vector2(0.035, 0.25)], space.brass, body)
	vase.position = Vector3(0.17, 0.538, 0.02)
	var globe := SphereMesh.new()
	globe.radius = 0.085
	globe.height = 0.17
	space.mesh_part(body, "ShelfGlobe", globe, Vector3(-0.13, 1.45, 0), space.study.material(Color("a58555")))
	space.cylinder(body, "GlobeFoot", Vector3(-0.13, 1.37, 0), 0.085, 0.03, ebony)
	batch_joinery(body)

func batch_joinery(body: Node3D) -> void:
	# Mouldings, rods and collars move as one bay. Each authored material has
	# one shadow mode; batch those pieces to reduce draws under every light.
	var batches := {}
	for part in body.get_children():
		if not part is MeshInstance3D or part.mesh == book_mesh: continue
		for i in range(part.mesh.get_surface_count()):
			var mat: Material = part.get_active_material(i)
			if not batches.has(mat):
				var surface := SurfaceTool.new()
				surface.begin(Mesh.PRIMITIVE_TRIANGLES)
				surface.set_material(mat)
				batches[mat] = {"surface": surface, "shadows": part.cast_shadow}
			batches[mat].surface.append_from(part.mesh, i, part.transform)
		part.free()
	for batch in batches.values():
		var node := MeshInstance3D.new()
		node.name = "Joinery"
		node.mesh = batch.surface.commit()
		node.cast_shadow = batch.shadows
		body.add_child(node)

func books(body: Node3D, pos: Vector3) -> void:
	# The imported set has twenty separate book meshes but only two materials.
	# Share one combined set across all shelves, preserving geometry and UVs.
	if book_mesh == null:
		var source: Node3D = load("res://assets/book_encyclopedia_set_01.gltf").instantiate()
		body.add_child(source)
		var surfaces := {}
		for part in source.find_children("*", "MeshInstance3D", true, false):
			for i in range(part.mesh.get_surface_count()):
				var mat: Material = part.get_active_material(i)
				if not surfaces.has(mat):
					var surface := SurfaceTool.new()
					surface.begin(Mesh.PRIMITIVE_TRIANGLES)
					surface.set_material(mat)
					surfaces[mat] = surface
				surfaces[mat].append_from(part.mesh, i, source.global_transform.affine_inverse() * part.global_transform)
		book_mesh = ArrayMesh.new()
		for surface in surfaces.values(): surface.commit(book_mesh)
		source.free()
	var node := MeshInstance3D.new()
	node.name = "Books"
	node.mesh = book_mesh
	body.add_child(node)
	node.position = pos
	node.scale = Vector3.ONE * 0.68

func banker_lamp(desk: Node3D, pos: Vector3) -> void:
	var lamp := Node3D.new()
	lamp.name = "BankerLamp"
	desk.add_child(lamp)
	lamp.position = pos
	space.cylinder(lamp, "Base", Vector3(0, 0.018, 0), 0.13, 0.035, space.brass)
	space.cylinder(lamp, "Stem", Vector3(0, 0.17, 0), 0.013, 0.3, space.brass)
	var enamel = space.study.material(Color("1c4838"))
	enamel.roughness = 0.22
	var shade = space.hall.lathe("EmeraldShade", [Vector2(0.06, 0.02), Vector2(0.15, 0.01), Vector2(0.19, -0.055), Vector2(0.2, -0.11)], enamel, lamp)
	shade.position.y = 0.34
	shade.scale.z = 0.55
	var light := OmniLight3D.new()
	lamp.add_child(light)
	light.position = Vector3(0, 0.22, 0)
	light.light_color = Color("ffe0a3")
	light.light_energy = 0.09
	light.omni_range = 1.0
	light.shadow_enabled = true
