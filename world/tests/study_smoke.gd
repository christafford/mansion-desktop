## Scene integration assertions. Injected input is not a human usability trial.
extends SceneTree

var failures := 0

func check(condition: bool, message: String) -> void:
	if not condition:
		push_error(message)
		failures += 1

func _initialize() -> void:
	call_deferred("run")

func key(player: Node, code: int, pressed: bool) -> void:
	var event := InputEventKey.new()
	event.physical_keycode = code
	event.pressed = pressed
	player._unhandled_input(event)

func run() -> void:
	if DisplayServer.get_name() == "headless":
		push_error("study_smoke requires a graphical display to verify pointer capture")
		quit(1)
		return
	var scene: PackedScene = load("res://scenes/main.tscn")
	var study := scene.instantiate()
	root.add_child(study)
	current_scene = study
	for frame in range(12):
		await process_frame
	var textured := 0
	for item in study.furniture:
		var meshes: Array[Node] = item.find_children("*", "MeshInstance3D", true, false)
		check(not meshes.is_empty(), str(item.name) + " has no imported meshes")
		for mesh in meshes:
			for surface in range(mesh.mesh.get_surface_count()):
				var mat = mesh.get_active_material(surface)
				if mat is StandardMaterial3D and mat.albedo_texture != null:
					textured += 1
	check(textured >= 6, "Furniture must have real texture materials")
	var player = study.get_node("Player")
	for frame in range(15):
		await physics_frame
	var start: Vector3 = player.position
	key(player, KEY_W, true)
	for frame in range(30):
		await physics_frame
	key(player, KEY_W, false)
	check(player.position.distance_to(start) > 0.8, "W does not move the player: %s -> %s" % [start, player.position])
	var stopped: Vector3 = player.position
	for frame in range(10):
		await physics_frame
	check(player.position.distance_to(stopped) < 0.06, "Movement continues after key release")
	# Walk into the back wall away from furniture; the capsule must stop inside.
	player.position = Vector3(-1.8, 0.05, -3.7)
	player.rotation.y = 0
	key(player, KEY_W, true)
	for frame in range(50):
		await physics_frame
	key(player, KEY_W, false)
	check(player.position.z > -4.25, "Player passed through room wall")
	check(player.position.z < -4.05, "Player failed to approach room wall")
	# Approach the actual imported chair, rather than a separately guessed box.
	player.position = Vector3(0.15, 0.05, 0.3)
	player.rotation.y = 0
	key(player, KEY_W, true)
	for frame in range(45):
		await physics_frame
	key(player, KEY_W, false)
	check(player.position.z > -0.65, "Player passed through imported chair")
	key(player, KEY_D, true)
	player._notification(Node.NOTIFICATION_APPLICATION_FOCUS_OUT)
	check(player._held.is_empty(), "Focus loss leaves held keys")
	var mouse := InputEventMouseButton.new()
	mouse.button_index = MOUSE_BUTTON_RIGHT
	mouse.pressed = true
	player._unhandled_input(mouse)
	check(Input.mouse_mode == Input.MOUSE_MODE_CAPTURED, "Right press does not capture")
	var motion := InputEventMouseMotion.new()
	motion.screen_relative = Vector2(100, 40)
	var yaw: float = player.rotation.y
	player._unhandled_input(motion)
	check(player.rotation.y < yaw - 0.1, "Mouse motion does not turn camera")
	mouse.pressed = false
	player._unhandled_input(mouse)
	check(Input.mouse_mode == Input.MOUSE_MODE_VISIBLE, "Right release does not release")
	key(player, KEY_HOME, true)
	key(player, KEY_HOME, false)
	check(player.position.is_equal_approx(player.SPAWN), "Home does not restore spawn")
	print("STUDY_SMOKE textured_surfaces=", textured, " failures=", failures)
	quit(1 if failures else 0)
