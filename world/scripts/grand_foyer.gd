## Authored vaulted foyer and usable spiral stair. All dimensions are meters.
extends RefCounted

const WIDTH := 20.0
const DEPTH := 24.0
const HEIGHT := 13.0
const GALLERY_Y := 5.6
const STAIR_CENTER := Vector3(0, 0, 12)
const STAIR_INNER := 1.8
const STAIR_OUTER := 3.7
const STAIR_START := -PI / 2
const STAIR_SWEEP := TAU * 1.25
const STEPS := 40
const RISE := GALLERY_Y / STEPS
var space: Node3D
var room: Node3D
var stone: Material
var floor_stone: Material
var trim: Material
var dark: Material
var gold: Material
var timber: Material
var baluster_mesh: Mesh
var baluster_poses: Array[Transform3D] = []

func build(builder: Node3D, parent: Node3D) -> void:
	space = builder
	room = parent
	room.get_viewport().use_occlusion_culling = true
	stone = stone_material(Color("b5ae98"))
	floor_stone = stone_material(Color("c9c7b9"), true)
	trim = space.study.material(Color("dbd1b5"))
	dark = space.study.material(Color("263e43"))
	gold = space.brass
	timber = space.study.material(Color("775537"), "res://assets/textures/walnut_veneer_4k_jpg.jpg")
	timber.roughness = 0.5
	var plaque := Node3D.new()
	room.add_child(plaque)
	plaque.position = Vector3(0,2.84,-0.19)
	plaque.rotation.y = PI
	space.part(plaque,"Nameplate",Vector3(1.7,0.28,0.025),Vector3.ZERO,dark)
	space.text(plaque,"THE GRAND FOYER",Vector3(0,0,0.02),30,0.0028)
	shell()
	colonnade()
	galleries()
	staircase()
	batch_balusters()
	rose_window()
	chandelier()
	lighting()
	for side in [-1, 1]:
		space.table(room, Vector3(side * 8.5, 0, 5.7), Vector2(2.0, 1.2), timber)
		space.prop(room, "potted_plant_02", Vector3(side * 8.7, 0, 2.3), 0, 1.3)
	room.set_meta("gallery_height", GALLERY_Y)
	room.set_meta("stair_start", STAIR_CENTER + Vector3(0, 0, -2.75))

func stone_material(color: Color, floor_value := false) -> ShaderMaterial:
	var mat := ShaderMaterial.new()
	mat.shader = preload("res://scripts/foyer_stone.gdshader")
	mat.set_shader_parameter("stone_color", color)
	mat.set_shader_parameter("floor_surface", floor_value)
	return mat

func box(label: String, size: Vector3, pos: Vector3, mat: Material, solid := true) -> MeshInstance3D:
	var mesh = space.part(room, label, size, pos, mat, solid)
	if label in ["EntranceWall", "EntranceHeader", "FarWall", "FoyerWall"]:
		# The tall foyer otherwise draws the entire furnished wing through its
		# solid entrance wall. Exact wall-sized occluders preserve the doorway.
		var blocker := OccluderInstance3D.new()
		blocker.name = label + "Occluder"
		blocker.occluder = BoxOccluder3D.new()
		blocker.occluder.size = size
		room.add_child(blocker)
		blocker.position = pos
	return mesh

func surface(mat: Material) -> SurfaceTool:
	var result := SurfaceTool.new()
	result.begin(Mesh.PRIMITIVE_TRIANGLES)
	result.set_material(mat)
	return result

func quad(s: SurfaceTool, a: Vector3, b: Vector3, c: Vector3, d: Vector3, normal: Vector3) -> void:
	# Godot front faces wind clockwise. Supply analytic normals for curved work.
	var points := [a, b, c, a, c, d]
	if (b - a).cross(c - a).dot(normal) > 0: points = [a, c, b, a, d, c]
	for point in points:
		s.set_normal(normal)
		s.set_uv(Vector2(point.x, point.z) * 0.4 if absf(normal.y) > 0.6 else Vector2(point.x + point.z, point.y) * 0.4)
		s.add_vertex(point)

func finish(s: SurfaceTool, label: String, solid := false, parent: Node3D = null) -> MeshInstance3D:
	var node := MeshInstance3D.new()
	node.name = label
	node.mesh = s.commit()
	(room if parent == null else parent).add_child(node)
	if solid: node.create_trimesh_collision()
	return node

func tube(label: String, path: Array[Vector3], radius: float, mat: Material, parent: Node3D = null) -> MeshInstance3D:
	var s := surface(mat)
	for i in range(path.size() - 1):
		var tangent := (path[i + 1] - path[i]).normalized()
		var right := tangent.cross(Vector3.UP).normalized()
		if right.length_squared() < 0.1: right = Vector3.RIGHT
		var up := tangent.cross(right).normalized()
		for j in range(12):
			var a := TAU * j / 12
			var b := TAU * (j + 1) / 12
			var na := right * cos(a) + up * sin(a)
			var nb := right * cos(b) + up * sin(b)
			quad(s, path[i] + na * radius, path[i + 1] + na * radius, path[i + 1] + nb * radius, path[i] + nb * radius, (na + nb).normalized())
	return finish(s, label, false, parent)

func arch(label: String, pos: Vector3, width: float, spring: float, rise: float, yaw := 0.0) -> void:
	var s := surface(trim)
	for i in range(32):
		var a := PI * i / 32
		var b := PI * (i + 1) / 32
		var ia := Vector3(cos(a) * width / 2, spring + sin(a) * rise, 0)
		var ib := Vector3(cos(b) * width / 2, spring + sin(b) * rise, 0)
		var oa := Vector3(cos(a) * (width / 2 + 0.23), spring + sin(a) * (rise + 0.23), 0)
		var ob := Vector3(cos(b) * (width / 2 + 0.23), spring + sin(b) * (rise + 0.23), 0)
		quad(s, ia, ib, ob, oa, Vector3.FORWARD * -1)
		quad(s, oa, ob, ob - Vector3(0, 0, 0.22), oa - Vector3(0, 0, 0.22), Vector3(cos((a+b)/2), sin((a+b)/2), 0))
		quad(s, ia, ia - Vector3(0, 0, 0.22), ib - Vector3(0, 0, 0.22), ib, -Vector3(cos((a+b)/2), sin((a+b)/2), 0))
	var node := finish(s, label)
	node.position = pos
	node.rotation.y = yaw

func arched_panel(pos: Vector3, width: float, bottom: float, spring: float, rise: float, mat: Material, yaw: float) -> void:
	var s := surface(mat)
	quad(s, Vector3(-width/2, bottom, 0), Vector3(width/2, bottom, 0), Vector3(width/2, spring, 0), Vector3(-width/2, spring, 0), Vector3.BACK)
	for i in range(32):
		var a := PI * i / 32
		var b := PI * (i + 1) / 32
		quad(s, Vector3(0, spring, 0), Vector3(cos(a)*width/2, spring + sin(a)*rise, 0), Vector3(cos(b)*width/2, spring + sin(b)*rise, 0), Vector3(0, spring, 0), Vector3.BACK)
	var node := finish(s, "ArchedRecess")
	node.position = pos
	node.rotation.y = yaw

func shell() -> void:
	box("FoyerFloor", Vector3(WIDTH, 0.24, DEPTH + 0.2), Vector3(0, -0.12, DEPTH/2), floor_stone)
	for x in [-10.0, 10.0]:
		box("FoyerWall", Vector3(0.3, 9.1, DEPTH), Vector3(x, 4.55, DEPTH/2), stone)
		box("StoneDado", Vector3(0.12, 0.75, DEPTH), Vector3(x - signf(x)*0.19, 0.375, DEPTH/2), dark)
		for y in [0.1, 0.8, 5.25, 8.9]:
			box("StringCourse", Vector3(0.24, 0.12, DEPTH), Vector3(x - signf(x)*0.22, y, DEPTH/2), trim, false)
	box("FarWall", Vector3(WIDTH, HEIGHT, 0.3), Vector3(0, HEIGHT/2, DEPTH), stone)
	for side in [-1, 1]:
		box("EntranceWall", Vector3(9.1, HEIGHT, 0.24), Vector3(side*5.45, HEIGHT/2, 0), stone)
	box("EntranceHeader", Vector3(1.8, HEIGHT - 2.5, 0.24), Vector3(0, (HEIGHT+2.5)/2, 0), stone)
	box("Threshold", Vector3(1.8, 0.012, 0.35), Vector3(0, 0.006, 0), gold, false)
	# A continuous elliptical barrel vault, with real ceiling collision.
	var vault := surface(stone)
	for i in range(64):
		var a := PI * i / 64
		var b := PI * (i + 1) / 64
		var p := Vector3(cos(a)*10, 9 + sin(a)*4, 0)
		var q := Vector3(cos(b)*10, 9 + sin(b)*4, 0)
		quad(vault, p, q, q + Vector3(0,0,DEPTH), p + Vector3(0,0,DEPTH), -Vector3(cos((a+b)/2), sin((a+b)/2), 0))
	var roof := finish(vault, "BarrelVault", true)
	roof.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_DOUBLE_SIDED
	for z in [1.0, 6.0, 11.0, 16.0, 21.0, 23.8]:
		var path: Array[Vector3] = []
		for i in range(65):
			var a := PI * i / 64
			path.append(Vector3(cos(a)*9.9, 8.95 + sin(a)*3.94, z))
		tube("VaultRib", path, 0.11, trim)
	for radius in [4.4, 4.5, 5.2]: space.ring(room, STAIR_CENTER + Vector3(0, 0.013, 0), radius, 0.018, gold)
	var compass := surface(dark)
	for i in range(16):
		var a := TAU * i / 16
		var tip := STAIR_CENTER + Vector3(cos(a)*4.35, 0.007, sin(a)*4.35)
		var l := STAIR_CENTER + Vector3(cos(a-0.15)*1.2, 0.007, sin(a-0.15)*1.2)
		var r := STAIR_CENTER + Vector3(cos(a+0.15)*1.2, 0.007, sin(a+0.15)*1.2)
		quad(compass, l, tip, r, l, Vector3.UP)
	finish(compass, "CompassInlay")

func colonnade() -> void:
	var profile := [Vector2(0.48,0), Vector2(0.48,0.2), Vector2(0.39,0.26), Vector2(0.36,0.42), Vector2(0.30,0.55), Vector2(0.33,2.2), Vector2(0.28,8.25), Vector2(0.36,8.35), Vector2(0.36,8.5), Vector2(0.51,8.6), Vector2(0.52,8.85)]
	for side in [-1, 1]:
		for z in [2.0, 7.0, 12.0, 17.0, 22.0]:
			var column = space.hall.lathe("ColossalColumn", profile, trim, room)
			column.position = Vector3(side*8.0, 0, z)
			column.create_trimesh_collision()
			box("CapitalAbacus", Vector3(1.12,0.18,1.12), Vector3(side*8.0,8.9,z), trim, false)
		for z in [4.5, 9.5, 14.5, 19.5]:
			var yaw := PI/2 if side < 0 else -PI/2
			var pos := Vector3(side*9.81, 0, z)
			arched_panel(pos, 2.7, 0.9, 2.8, 1.35, dark, yaw)
			arch("LowerArcade", pos + Vector3(-side*0.035,0,0), 2.75, 2.8, 1.4, yaw)
			var glass = space.study.material(Color("719fa7"))
			glass.emission_enabled = true
			glass.emission = Color("83b1bd")
			glass.emission_energy_multiplier = 0.3
			arched_panel(pos, 2.6, 6.0, 8.0, 1.35, glass, yaw)
			arch("UpperArcade", pos + Vector3(-side*0.04,0,0), 2.65, 8.0, 1.4, yaw)
			for offset in [-1.5, 1.5]:
				box("RecessJamb", Vector3(0.19, 3.0, 0.23), Vector3(side*9.77, 1.85, z+offset), trim, false)

func galleries() -> void:
	box("FrontGallery", Vector3(20,0.35,4.0), Vector3(0,GALLERY_Y-0.175,2.0), floor_stone)
	box("RearGallery", Vector3(20,0.35,3.0), Vector3(0,GALLERY_Y-0.175,22.5), floor_stone)
	for side in [-1, 1]:
		box("SideGallery", Vector3(2.7,0.35,17), Vector3(side*8.65,GALLERY_Y-0.175,12.5), floor_stone)
		if side < 0:
			balustrade(Vector3(-7.3,GALLERY_Y,4), Vector3(-7.3,GALLERY_Y,21))
		else:
			balustrade(Vector3(7.3,GALLERY_Y,4), Vector3(7.3,GALLERY_Y,12))
			balustrade(Vector3(7.3,GALLERY_Y,13.4), Vector3(7.3,GALLERY_Y,21))
	balustrade(Vector3(-7.3,GALLERY_Y,4), Vector3(7.3,GALLERY_Y,4))
	balustrade(Vector3(-7.3,GALLERY_Y,21), Vector3(7.3,GALLERY_Y,21))
	# Top flight meets a landing, then turns onto the east gallery bridge.
	box("StairLanding", Vector3(2.1,0.3,1.4), Vector3(2.75,GALLERY_Y-0.15,12.7), floor_stone)
	box("GalleryBridge", Vector3(3.7,0.3,1.4), Vector3(5.55,GALLERY_Y-0.15,12.7), floor_stone)
	balustrade(Vector3(1.7,GALLERY_Y,12), Vector3(1.7,GALLERY_Y,13.4))
	balustrade(Vector3(1.7,GALLERY_Y,13.4), Vector3(7.3,GALLERY_Y,13.4))
	balustrade(Vector3(3.8,GALLERY_Y,12), Vector3(7.3,GALLERY_Y,12))

func rail_post(pos: Vector3, height := 1.0) -> void:
	if baluster_mesh == null:
		var profile := [Vector2(0.035,0),Vector2(0.047,0.06),Vector2(0.024,0.15),Vector2(0.021,0.68),Vector2(0.036,0.76),Vector2(0.026,0.92),Vector2(0.028,1.0)]
		var prototype = space.hall.lathe("BalusterPrototype", profile, gold, room)
		baluster_mesh = prototype.mesh
		prototype.free()
	baluster_poses.append(Transform3D(Basis.IDENTITY.scaled(Vector3(1,height,1)),pos))

func batch_balusters() -> void:
	# Share the turned profile in one draw; collision guards remain independent.
	var batch := MultiMeshInstance3D.new()
	batch.name = "TurnedBalusters"
	batch.multimesh = MultiMesh.new()
	batch.multimesh.transform_format = MultiMesh.TRANSFORM_3D
	batch.multimesh.mesh = baluster_mesh
	batch.multimesh.instance_count = baluster_poses.size()
	batch.material_override = gold
	for i in range(baluster_poses.size()): batch.multimesh.set_instance_transform(i,baluster_poses[i])
	room.add_child(batch)

func guard(a: Vector3, b: Vector3) -> void:
	# Solid guard follows the railing; no path through gaps between its bars.
	var body := StaticBody3D.new()
	body.name = "BalustradeGuard"
	body.add_to_group("foyer_guards")
	room.add_child(body)
	body.position = (a+b)/2 + Vector3(0,0.5,0)
	body.rotation.y = atan2((b-a).x, (b-a).z)
	var shape := CollisionShape3D.new()
	var bounds := BoxShape3D.new()
	bounds.size = Vector3(0.07, 1.05 + absf(b.y-a.y), Vector2(b.x-a.x,b.z-a.z).length()+0.02)
	shape.shape = bounds
	body.add_child(shape)

func balustrade(a: Vector3, b: Vector3) -> void:
	var length := a.distance_to(b)
	var count := ceili(length / 0.3)
	for i in range(count+1): rail_post(a.lerp(b, float(i)/count))
	tube("GalleryHandrail", [a+Vector3.UP, b+Vector3.UP], 0.055, timber)
	guard(a,b)

static func stair_point(angle: float, radius: float, y: float) -> Vector3:
	return STAIR_CENTER + Vector3(cos(angle)*radius, y, sin(angle)*radius)

static func stair_height(angle: float) -> float:
	return clampf((angle-STAIR_START)/STAIR_SWEEP * GALLERY_Y + RISE, 0, GALLERY_Y)

func staircase() -> void:
	var treads := surface(timber)
	var masonry := surface(trim)
	for i in range(STEPS):
		var a := STAIR_START + STAIR_SWEEP*i/STEPS
		var b := STAIR_START + STAIR_SWEEP*(i+1)/STEPS
		var y := (i+1)*RISE
		# Subdivide the curved edges, retaining an actual flat tread per riser.
		for j in range(4):
			var u := lerpf(a,b,j/4.0)
			var v := lerpf(a,b,(j+1)/4.0)
			var ia := stair_point(u,STAIR_INNER,y)
			var ib := stair_point(v,STAIR_INNER,y)
			var oa := stair_point(u,STAIR_OUTER,y)
			var ob := stair_point(v,STAIR_OUTER,y)
			quad(treads,ia,ib,ob,oa,Vector3.UP)
			# Continuous sloping soffit joins adjacent steps without open seams.
			var drop_a := Vector3.UP * (j / 4.0 * RISE - 0.32)
			var drop_b := Vector3.UP * ((j + 1) / 4.0 * RISE - 0.32)
			quad(masonry,ia+drop_a,oa+drop_a,ob+drop_b,ib+drop_b,Vector3.DOWN)
			var radial := Vector3(cos((u+v)/2),0,sin((u+v)/2))
			quad(masonry,oa,ob,ob+drop_b,oa+drop_a,radial)
			quad(masonry,ia,ia+drop_a,ib+drop_b,ib,-radial)
		var inner := stair_point(a,STAIR_INNER,y)
		var outer := stair_point(a,STAIR_OUTER,y)
		quad(masonry,inner,outer,outer-Vector3(0,RISE,0),inner-Vector3(0,RISE,0),Vector3(sin(a),0,-cos(a)))
		tube("BrassTreadNosing",[inner+Vector3(0,0.006,0),outer+Vector3(0,0.006,0)],0.014,gold)
		for radius in [STAIR_INNER, STAIR_OUTER]:
			rail_post(stair_point((a+b)/2,radius,y),stair_height((a+b)/2)+1-y)
	for mesh in [finish(treads,"SpiralTreads",true),finish(masonry,"HelicalStoneWaist",true)]:
		# The character uses the continuous guide exclusively, so its capsule
		# cannot snag a tread's vertical edge during a turn or descent.
		for body in mesh.find_children("*","StaticBody3D",true,false):
			space.study.get_node("Player").add_collision_exception_with(body)
	# Character-only smooth collision avoids snagging on risers. Panels and
	# furniture still land on the actual flat treads (layer 1).
	var ramp := surface(trim)
	var start := STAIR_START - STAIR_SWEEP/STEPS
	for i in range(STEPS*4+4):
		var a := start + STAIR_SWEEP/STEPS * i/4.0
		var b := start + STAIR_SWEEP/STEPS * (i+1)/4.0
		quad(ramp,stair_point(a,STAIR_INNER,stair_height(a)),stair_point(b,STAIR_INNER,stair_height(b)),stair_point(b,STAIR_OUTER,stair_height(b)),stair_point(a,STAIR_OUTER,stair_height(a)),Vector3.UP)
		quad(ramp,stair_point(a,STAIR_INNER,stair_height(a)-0.32),stair_point(a,STAIR_OUTER,stair_height(a)-0.32),stair_point(b,STAIR_OUTER,stair_height(b)-0.32),stair_point(b,STAIR_INNER,stair_height(b)-0.32),Vector3.DOWN)
	var guide := finish(ramp,"WalkingRamp",true)
	guide.visible = false
	for body in guide.find_children("*","StaticBody3D",true,false): body.collision_layer = 8
	for radius in [STAIR_INNER,STAIR_OUTER]:
		var path: Array[Vector3] = []
		for i in range(STEPS*4+1):
			var angle := STAIR_START + STAIR_SWEEP*i/(STEPS*4)
			var point := stair_point(angle,radius,stair_height(angle))
			path.append(point+Vector3.UP)
			if i > 0: guard(path[i-1]-Vector3.UP,point)
		tube("SpiralHandrail",path,0.06,timber)
	# Newel posts make the first step and landing legible from the entrance.
	for angle in [STAIR_START,STAIR_START+STAIR_SWEEP]:
		for radius in [STAIR_INNER,STAIR_OUTER]:
			var base := stair_point(angle,radius,stair_height(angle))
			space.cylinder(room,"Newel",base+Vector3(0,0.52,0),0.09,1.04,gold)
			var ball := SphereMesh.new()
			ball.radius=0.13
			ball.height=0.26
			space.mesh_part(room,"NewelFinial",ball,base+Vector3(0,1.08,0),gold)

func rose_window() -> void:
	var window := Node3D.new()
	window.name = "RoseWindow"
	room.add_child(window)
	window.position = Vector3(0,8.5,23.8)
	window.rotation.y = PI
	for radius in [0.48,1.0,2.22,2.5]: space.ring(window,Vector3.ZERO,radius,0.045,gold,Vector3(PI/2,0,0))
	for i in range(16):
		var mat = space.study.material(Color(["91bec4","c5aa74","476f8f"][i%3]))
		mat.emission_enabled=true
		mat.emission=mat.albedo_color
		mat.emission_energy_multiplier=0.55
		var s := surface(mat)
		var a := TAU*i/16
		var b := TAU*(i+1)/16
		quad(s,Vector3(cos(a)*0.5,sin(a)*0.5,0),Vector3(cos(a)*2.22,sin(a)*2.22,0),Vector3(cos(b)*2.22,sin(b)*2.22,0),Vector3(cos(b)*0.5,sin(b)*0.5,0),Vector3.BACK)
		finish(s,"ColoredGlass",false,window)
		tube("LeadSpoke",[Vector3.ZERO,Vector3(cos(a)*2.25,sin(a)*2.25,0.03)],0.022,gold,window)
	for side in [-1,1]:
		box("VelvetBanner",Vector3(1.7,4.6,0.04),Vector3(side*4.5,8.3,23.76),dark,false)
		for x in [-0.8,0.8]: box("BannerBraid",Vector3(0.035,4.5,0.012),Vector3(side*4.5+x,8.3,23.72),gold,false)
		space.ring(room,Vector3(side*4.5,8.6,23.7),0.48,0.04,gold,Vector3(PI/2,0,0))

func chandelier() -> void:
	for radius in [1.2,2.5]: space.ring(room,Vector3(0,9.3,12),radius,0.055,gold)
	for i in range(16):
		var a := TAU*i/16
		var p := Vector3(cos(a)*2.5,9.3,12+sin(a)*2.5)
		tube("ChandelierArm",[Vector3(cos(a)*1.2,9.3,12+sin(a)*1.2),p],0.025,gold)
		space.cylinder(room,"CandleCup",p,0.11,0.07,gold)
		space.cylinder(room,"IvoryCandle",p+Vector3(0,0.19,0),0.04,0.35,trim)
		var glow = space.study.material(Color("ffe0a1"))
		glow.emission_enabled=true
		glow.emission=Color("ffd991")
		glow.emission_energy_multiplier=2
		var flame := SphereMesh.new()
		flame.radius=0.035
		flame.height=0.16
		space.mesh_part(room,"CandleFlame",flame,p+Vector3(0,0.41,0),glow)
	for i in range(4):
		var a := TAU*i/4
		tube("Suspension",[Vector3(cos(a)*1.2,9.3,12+sin(a)*1.2),Vector3(0,12.85,12)],0.022,gold)

func lighting() -> void:
	var key := SpotLight3D.new()
	room.add_child(key)
	key.position=Vector3(0,10.7,12)
	key.rotation.x=-PI/2
	key.light_color=Color("ffe0b0")
	key.light_energy=1.2
	key.spot_range=23
	key.spot_angle=67
	key.shadow_enabled=true
	key.shadow_bias=0.05
	# Back-face shadow depths avoid self-shadow striping on tall stone columns.
	# The open vault explicitly casts on both sides.
	key.shadow_reverse_cull_face=true
	var window_light := SpotLight3D.new()
	room.add_child(window_light)
	window_light.position=Vector3(0,10,23)
	window_light.look_at(room.to_global(Vector3(0,1,11)))
	window_light.light_color=Color("b8daed")
	window_light.light_energy=0.85
	window_light.spot_range=26
	window_light.spot_angle=58
	window_light.shadow_enabled=true
	window_light.shadow_bias=0.05
	window_light.shadow_reverse_cull_face=true
	for pos in [Vector3(-6,5,4),Vector3(6,5,4),Vector3(-8,8,15),Vector3(8,8,15)]:
		var fill := OmniLight3D.new()
		room.add_child(fill)
		fill.position=pos
		fill.light_color=Color("d0dfdd")
		fill.light_energy=0.4
		fill.omni_range=15
