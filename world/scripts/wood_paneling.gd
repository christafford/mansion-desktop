## Full-height walnut veneer panels with recessed joints and solid joinery.
## Uses the existing verified Poly Haven walnut photo, with meter-scaled UVs.
extends Node3D

var study: Node3D

func _ready() -> void:
	study = get_parent()
	run(8, 3.2, Vector3(0, 1.6, -4.415), 0)
	run(8, 3.2, Vector3(0, 1.6, 4.415), PI)
	run(9, 3.2, Vector3(3.915, 1.6, 0), -PI / 2)
	run(9, 0.85, Vector3(-3.915, 0.425, 0), PI / 2)
	run(9, 0.6, Vector3(-3.915, 2.9, 0), PI / 2)
	run(2.6, 1.75, Vector3(-3.915, 1.725, -3.2), PI / 2)
	run(3.4, 1.75, Vector3(-3.915, 1.725, 2.8), PI / 2)

func run(width: float, height: float, center: Vector3, yaw: float) -> void:
	var wall := Node3D.new()
	add_child(wall)
	wall.position = center
	wall.rotation.y = yaw
	var edge = study.material(Color("493827"))
	var joinery = study.material(Color("644d35"))
	var horizontal := center.dot(wall.basis.x)
	var start := -width / 2
	while start < width / 2 - 0.001:
		var board := floori((start + horizontal + 0.001) / 0.65)
		var end := minf((board + 1) * 0.65 - horizontal, width / 2)
		var length := end - start
		var edge_mesh: MeshInstance3D = study.box("VeneerEdge", Vector3(length - 0.008, height, 0.022), Vector3((start + end) / 2, 0, 0), edge, false, wall)
		edge_mesh.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		var face := MeshInstance3D.new()
		face.name = "WalnutPanel"
		var quad := QuadMesh.new()
		quad.size = Vector2(length - 0.01, height)
		face.mesh = quad
		# The solid wall behind supplies occlusion; layered veneer must not self-shadow.
		face.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		var mat = study.material(Color(0.85, 0.77, 0.64), "res://assets/textures/walnut_veneer_4k_jpg.jpg")
		mat.roughness = 0.57
		mat.uv1_scale = Vector3(length / 2.0, height / 2.0, 1)
		mat.uv1_offset = Vector3(fposmod(board * 0.271, 1), (center.y - height / 2) / 2.0, 0)
		face.material_override = mat
		# The front is separate from BoxMesh's texture atlas, preserving the grain.
		wall.add_child(face)
		face.position = Vector3((start + end) / 2, 0, 0.012)
		start = end
	for world_y in [0.17, 1.05, 3.1]:
		if world_y > center.y - height / 2 + 0.04 and world_y < center.y + height / 2 - 0.04:
			study.box("PanelRail", Vector3(width, 0.045, 0.035), Vector3(0, world_y - center.y, 0.017), joinery, false, wall)
