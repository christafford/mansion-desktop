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
	# A/D always strafe, independent of capture, and diagonals keep walking speed.
	for looking in [false, true]:
		for test in [[KEY_W, Vector3.FORWARD], [KEY_S, Vector3.BACK], [KEY_A, Vector3.LEFT], [KEY_D, Vector3.RIGHT]]:
			await travel(player, [test[0]], test[1], 2.4, looking)
	await travel(player, [KEY_W, KEY_D], Vector3(1, 0, -1).normalized(), 2.4)
	await travel(player, [KEY_W, KEY_S], Vector3.ZERO, 0)
	await travel(player, [KEY_A, KEY_D], Vector3.ZERO, 0)
	await travel(player, [KEY_W, KEY_SHIFT], Vector3.FORWARD, 4.0)
	await travel(player, [KEY_W, KEY_CTRL], Vector3.FORWARD, 1.0)
	await travel(player, [KEY_W, KEY_CTRL, KEY_SHIFT], Vector3.FORWARD, 1.0)
	key(player, KEY_M, true)
	key(player, KEY_M, false)
	await travel(player, [KEY_W, KEY_SHIFT], Vector3.FORWARD, 1.0)
	key(player, KEY_M, true)
	key(player, KEY_M, false)
	await travel(player, [KEY_W], Vector3.LEFT, 2.4, true, PI / 2, 1.2)

	player.return_to_spawn()
	var mouse := InputEventMouseButton.new()
	mouse.button_index = MOUSE_BUTTON_RIGHT
	mouse.pressed = true
	player._unhandled_input(mouse)
	check(Input.mouse_mode == Input.MOUSE_MODE_CAPTURED, "Right press does not capture")
	var motion := InputEventMouseMotion.new()
	motion.screen_relative = Vector2(100, 40)
	var yaw: float = player.rotation.y
	var pitch: float = player.camera.rotation.x
	player._unhandled_input(motion)
	check(player.rotation.y < yaw - 0.1, "Mouse right does not turn right")
	check(player.camera.rotation.x < pitch - 0.05, "Mouse down does not look down")
	var aimed_yaw: float = player.rotation.y
	var aimed_pitch: float = player.camera.rotation.x
	key(player, KEY_W, true)
	mouse.pressed = false
	player._unhandled_input(mouse)
	check(Input.mouse_mode == Input.MOUSE_MODE_VISIBLE, "Right release does not free pointer")
	check(player._held.has(KEY_W), "Right release interrupts held walking")
	key(player, KEY_W, false)
	player._unhandled_input(motion)
	await create_timer(0.3).timeout
	check(is_equal_approx(player.rotation.y, aimed_yaw), "Released view springs back horizontally")
	check(is_equal_approx(player.camera.rotation.x, aimed_pitch), "Released view springs back vertically")
	# Repeated grabs start from the last view, including upward and leftward look.
	for repeat in range(3):
		mouse.pressed = true
		player._unhandled_input(mouse)
		check(is_equal_approx(player.rotation.y, aimed_yaw), "Grabbing resets heading")
		motion.screen_relative = Vector2(-50, -40)
		player._unhandled_input(motion)
		check(player.rotation.y > aimed_yaw and player.camera.rotation.x > aimed_pitch, "Mouse left/up is inverted")
		aimed_yaw = player.rotation.y
		aimed_pitch = player.camera.rotation.x
		mouse.pressed = false
		player._unhandled_input(mouse)
		await create_timer(0.3).timeout
		check(is_equal_approx(player.rotation.y, aimed_yaw) and is_equal_approx(player.camera.rotation.x, aimed_pitch), "View drifts after re-grab/release")
	mouse.pressed = true
	player._unhandled_input(mouse)
	motion.screen_relative = Vector2(0, -10000)
	player._unhandled_input(motion)
	check(is_equal_approx(player.camera.rotation.x, 1.35), "Upward pitch flips camera")
	motion.screen_relative = Vector2(0, 20000)
	player._unhandled_input(motion)
	check(is_equal_approx(player.camera.rotation.x, -1.35), "Downward pitch flips camera")

	# Real event routing must release capture even over a GUI control.
	var blocker := LineEdit.new()
	blocker.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	root.add_child(blocker)
	blocker.grab_focus()
	key(player, KEY_W, true)
	mouse.pressed = false
	mouse.position = Vector2(40, 40)
	Input.parse_input_event(mouse)
	var release := InputEventKey.new()
	release.physical_keycode = KEY_W
	Input.parse_input_event(release)
	await process_frame
	check(not player._look_held and Input.mouse_mode == Input.MOUSE_MODE_VISIBLE, "GUI swallows mouse-look release")
	check(player._held.is_empty(), "GUI swallows walking release")
	blocker.queue_free()
	await process_frame

	mouse.pressed = true
	player._unhandled_input(mouse)
	key(player, KEY_D, true)
	player._notification(Node.NOTIFICATION_APPLICATION_FOCUS_OUT)
	check(player._held.is_empty() and player.velocity == Vector3.ZERO, "Focus loss leaves held movement")
	check(not player._look_held and Input.mouse_mode == Input.MOUSE_MODE_VISIBLE, "Focus loss leaves capture")
	key(player, KEY_W, true)
	player._unhandled_input(mouse)
	check(player._held.is_empty() and not player._look_held, "Unfocused input starts navigation")
	player._notification(Node.NOTIFICATION_APPLICATION_FOCUS_IN)
	player._unhandled_input(mouse)
	key(player, KEY_W, true)
	key(player, KEY_ESCAPE, true)
	check(player._held.is_empty() and not player._look_held, "Escape leaves movement/capture")
	player._unhandled_input(mouse)
	key(player, KEY_D, true)
	key(player, KEY_HOME, true)
	check(player.position.is_equal_approx(player.SPAWN), "Home does not restore spawn")
	check(is_equal_approx(player.camera.rotation.x, player.DEFAULT_PITCH), "Home does not restore pitch")
	check(player._held.is_empty() and not player._look_held, "Home leaves held movement/capture")
	player.application_mode = true
	player._unhandled_input(mouse)
	key(player, KEY_W, true)
	key(player, KEY_SHIFT, true)
	check(player._held.is_empty() and not player._look_held, "Application mode accepts navigation")
	player.application_mode = false
	print("STUDY_SMOKE textured_surfaces=", textured, " failures=", failures)
	quit(1 if failures else 0)

func travel(player: Node, keys: Array, expected_direction: Vector3, expected_speed: float, looking := false, yaw := 0.0, pitch := -0.08) -> void:
	player.release_pointer()
	player.position = Vector3(1.5, 0.05, 2.4)
	player.rotation.y = yaw
	player.camera.rotation.x = pitch
	for frame in range(4): await physics_frame
	var start: Vector3 = player.position
	if looking:
		var mouse := InputEventMouseButton.new()
		mouse.button_index = MOUSE_BUTTON_RIGHT
		mouse.pressed = true
		player._unhandled_input(mouse)
	for code in keys: key(player, code, true)
	for frame in range(12): await physics_frame
	var horizontal: Vector3 = player.velocity * Vector3(1, 0, 1)
	check(horizontal.is_equal_approx(expected_direction * expected_speed), "Wrong movement velocity for %s (look=%s): %s" % [keys, looking, horizontal])
	check(is_equal_approx(player.rotation.y, yaw), "Walking keys changed heading")
	check(absf(player.position.y - start.y) < 0.03, "Camera pitch lifts player off the floor")
	var offset: Vector3 = (player.position - start) * Vector3(1, 0, 1)
	if expected_speed > 0:
		check(offset.dot(expected_direction) > expected_speed * 0.1, "Walking did not translate in the expected direction")
	else:
		check(offset.length() < 0.01, "Opposing keys did not cancel")
	for code in keys: key(player, code, false)
	for frame in range(2): await physics_frame
	check((player.velocity * Vector3(1, 0, 1)).length() < 0.01, "Walking coasts after release")
	player.release_pointer()
