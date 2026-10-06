## Original room details: fluorescent fittings and a restrained home-office gallery.
extends Node3D

var study: Node3D
var fixtures: Array[SpotLight3D] = []
var tubes: Array[StandardMaterial3D] = []
var elapsed := 0.0

func _ready() -> void:
	study = get_parent()
	for z in [-1.8, 1.8]: fixture(Vector3(0, 3.12, z))
	print_frame("Fern", Vector3(-1.25, 2.05, -4.39), "res://art/fern.svg")
	print_frame("Ginkgo", Vector3(0.05, 2.05, -4.39), "res://art/ginkgo.svg")
	clock_face(Vector3(1.5, 2.52, -4.38))
	noticeboard()

func part(label: String, size: Vector3, pos: Vector3, mat: Material, parent: Node3D) -> MeshInstance3D:
	return study.box(label, size, pos, mat, false, parent)

func cylinder(label: String, radius: float, height: float, pos: Vector3, mat: Material, parent: Node3D) -> MeshInstance3D:
	var node := MeshInstance3D.new()
	node.name = label
	var mesh := CylinderMesh.new()
	mesh.top_radius = radius
	mesh.bottom_radius = radius
	mesh.height = height
	mesh.radial_segments = 32
	node.mesh = mesh
	node.material_override = mat
	parent.add_child(node)
	node.position = pos
	return node

func fixture(pos: Vector3) -> void:
	var fitting := Node3D.new()
	fitting.name = "Fluorescent%d" % fixtures.size()
	add_child(fitting)
	fitting.position = pos
	var enamel = study.material(Color("deded5"))
	enamel.metallic = 0.35
	enamel.roughness = 0.32
	var silver = study.material(Color("747c79"))
	silver.metallic = 0.8
	part("Housing", Vector3(1.38, 0.09, 0.42), Vector3.ZERO, enamel, fitting)
	part("Reflector", Vector3(1.29, 0.012, 0.36), Vector3(0, -0.051, 0), silver, fitting)
	var phosphor = study.material(Color("f4fff1"))
	phosphor.emission_enabled = true
	phosphor.emission = Color("e6f5dd")
	phosphor.emission_energy_multiplier = 2.0
	tubes.append(phosphor)
	for z in [-0.105, 0.105]:
		var tube := cylinder("FluorescentTube", 0.018, 1.18, Vector3(0, -0.078, z), phosphor, fitting)
		tube.rotation.z = PI / 2
		for x in [-0.615, 0.615]:
			part("Socket", Vector3(0.045, 0.065, 0.052), Vector3(x, -0.067, z), enamel, fitting)
	for z in [-0.21, 0.21]:
		part("ReflectorLip", Vector3(1.38, 0.06, 0.015), Vector3(0, -0.062, z), enamel, fitting)
	for x in [-0.43, 0.43]:
		part("TubeGuard", Vector3(0.012, 0.013, 0.4), Vector3(x, -0.103, 0), silver, fitting)
	var light := SpotLight3D.new()
	light.name = "Downlight"
	light.position = Vector3(0, -0.14, 0)
	light.rotation.x = -PI / 2
	light.light_color = Color("edf5e5")
	light.spot_range = 7
	light.spot_angle = 74
	light.spot_angle_attenuation = 0.45
	light.spot_attenuation = 0.7
	light.light_energy = 0.55
	light.shadow_enabled = true
	light.shadow_bias = 0.025
	light.shadow_normal_bias = 1.0
	fitting.add_child(light)
	fixtures.append(light)

func flicker(time: float, index: int) -> float:
	# Small asynchronous ballast fluctuations, not a periodic on/off strobe.
	var t := time + index * 9.13
	return 0.985 + 0.007 * sin(t * 17.3) * sin(t * 3.7) + 0.008 * sin(t * 0.71) - 0.025 * pow(maxf(0, sin(t * 1.17)), 32)

func _process(delta: float) -> void:
	elapsed += delta
	for i in range(fixtures.size()):
		var strength := flicker(elapsed, i)
		fixtures[i].light_energy = 0.55 * strength
		tubes[i].emission_energy_multiplier = 2.0 * strength

func frame_panel(label: String, size: Vector2, pos: Vector3, mat: Material, yaw := 0.0) -> Node3D:
	var frame := Node3D.new()
	frame.name = label
	add_child(frame)
	frame.position = pos
	frame.rotation.y = yaw
	var oak = study.material(Color("443b2b"))
	part("Backing", Vector3(size.x, size.y, 0.035), Vector3.ZERO, oak, frame)
	part("Face", Vector3(size.x - 0.045, size.y - 0.045, 0.008), Vector3(0, 0, 0.023), mat, frame)
	for x in [-size.x / 2, size.x / 2]:
		part("FrameRail", Vector3(0.025, size.y + 0.025, 0.055), Vector3(x, 0, 0.012), oak, frame)
	for y in [-size.y / 2, size.y / 2]:
		part("FrameRail", Vector3(size.x, 0.025, 0.055), Vector3(0, y, 0.012), oak, frame)
	return frame

func print_frame(label: String, pos: Vector3, texture: String) -> void:
	var mat = study.material(Color.WHITE, texture)
	var frame := frame_panel(label, Vector2(0.95, 1.2), pos, study.material(Color("eae4d1")))
	var print_mesh := MeshInstance3D.new()
	print_mesh.name = "Artwork"
	var quad := QuadMesh.new()
	quad.size = Vector2(0.905, 1.155)
	print_mesh.mesh = quad
	print_mesh.material_override = mat
	frame.add_child(print_mesh)
	print_mesh.position.z = 0.029

func clock_face(pos: Vector3) -> void:
	var frame := Node3D.new()
	frame.name = "WallClock"
	add_child(frame)
	frame.position = pos
	var dark = study.material(Color("263a36"))
	var rim := cylinder("Case", 0.22, 0.045, Vector3.ZERO, dark, frame)
	rim.rotation.x = PI / 2
	var face := cylinder("Dial", 0.203, 0.008, Vector3(0, 0, 0.027), study.material(Color("ebe5ce")), frame)
	face.rotation.x = PI / 2
	for i in range(12):
		var a := i * TAU / 12
		var tick := part("Hour", Vector3(0.009, 0.025, 0.003), Vector3(sin(a) * 0.175, cos(a) * 0.175, 0.034), dark, frame)
		tick.rotation.z = -a
	# Clock hands show local system time when the room is opened.
	var time := Time.get_time_dict_from_system()
	for spec in [[0.085, (time.hour % 12 + time.minute / 60.0) * TAU / 12], [0.135, time.minute * TAU / 60]]:
		var hand := part("Hand", Vector3(0.012, spec[0], 0.006), Vector3(sin(spec[1]) * spec[0] / 2, cos(spec[1]) * spec[0] / 2, 0.04), dark, frame)
		hand.rotation.z = -spec[1]

func noticeboard() -> void:
	var board := frame_panel("Noticeboard", Vector2(1.25, 0.85), Vector3(3.89, 1.85, 0.1), study.material(Color("93734b")), -PI / 2)
	var ink = study.material(Color("52665d"))
	for i in range(4):
		var note := Node3D.new()
		board.add_child(note)
		note.position = Vector3(-0.43 + (i % 3) * 0.4, 0.1 if i < 3 else -0.22, 0.034 + i * 0.001)
		note.rotation.z = [-0.08, 0.06, -0.04, 0.1][i]
		part("Paper", Vector3(0.28, 0.29, 0.003), Vector3.ZERO, study.material(Color("ece5ca") if i != 1 else Color("b8c8b7")), note)
		for row in range(5):
			part("HandwrittenLine", Vector3(0.15 - (row % 3) * 0.026, 0.003, 0.001), Vector3(-0.025, 0.065 - row * 0.031, 0.003), ink, note)
		var pin := cylinder("BrassPin", 0.011, 0.009, Vector3(0, 0.12, 0.008), study.material(Color("b18a47")), note)
		pin.rotation.x = PI / 2
