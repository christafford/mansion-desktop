## Bounded P22-T02 startup/navigation check; not the village integration gate.
extends "res://tests/application_objects.gd"

func frames(count: int) -> void:
	for i in range(count): await physics_frame

func run() -> void:
	output_dir = ProjectSettings.globalize_path("res://../.tools/outdoor-startup-test").simplify_path()
	DirAccess.make_dir_recursive_absolute(output_dir)
	# Resolve the project setting: a passing test must exercise the actual default.
	study = load(ProjectSettings.get_setting("application/run/main_scene")).instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	objects = study.application_objects
	var player = study.get_node("Player")
	check(study.name == "Outdoor", "Default startup is not outdoors")
	for script in ["study", "hallway", "elsewhere_rooms", "grand_foyer"]:
		check(not ResourceLoader.has_cached("res://scripts/" + script + ".gd"), "Default startup loads legacy geometry: " + script)
	check(get_nodes_in_group("pushable_furniture").is_empty(), "Legacy furnishings are active")
	check(get_nodes_in_group("hallway_doors").is_empty(), "Legacy doors are active")
	var env: Environment = study.get_node("WorldEnvironment").environment
	check(env.background_mode == Environment.BG_SKY and env.sky != null, "Outdoor sky missing")
	check(env.tonemap_mode == Environment.TONE_MAPPER_FILMIC and env.tonemap_exposure == 1.0 and env.tonemap_white == 1.0, "Client screen color contract changed")
	check(env.ambient_light_source == Environment.AMBIENT_SOURCE_COLOR and study.get_node("Sun").shadow_enabled, "Outdoor lighting missing")
	var deadline := Time.get_ticks_msec() + 12000
	while objects.bindings.is_empty() and Time.get_ticks_msec() < deadline: await process_frame
	check(terminal.session != null and terminal.updates > 0 and not objects.bindings.is_empty(), "Outdoor startup did not map its real terminal")
	if failures:
		await study.desktop_runtime.shutdown()
		quit(1)
		return
	var window: int = terminal.handle
	var object: Node3D = objects.bindings[window]
	var identity: String = object.entity_id
	check(not terminal.screen.is_visible_in_tree() and object.is_visible_in_tree(), "Selected preview duplicates the live world panel")
	if DisplayServer.get_name() != "headless":
		root.grab_focus()
		await settle()
		await capture("arrival.png")
	await frames(15)
	check(player.is_on_floor() and absf(player.position.y) < 0.03, "Spawn does not settle safely on ground")
	var spawn: Vector3 = player.position
	await key(KEY_A, true)
	await frames(30)
	await key(KEY_A, false)
	check(player.position.x < spawn.x - 1.0 and player.is_on_floor(), "Outdoor movement does not walk on ground")
	var stopped: Vector3 = player.position
	await frames(10)
	check(player.position.distance_to(stopped) < 0.01, "Walking continues after release")
	# Preserve the owner's existing jump and prove the startup boundary contains it.
	await tap(KEY_SPACE)
	var peak := 0.0
	for i in range(75):
		await physics_frame
		peak = maxf(peak, player.position.y)
	check(peak > 0.8 and player.is_on_floor() and absf(player.position.y) < 0.03, "Jump or landing regressed")
	for edge in [[Vector3(0, 0.05, -11), 0.0], [Vector3(0, 0.05, 11), PI], [Vector3(11, 0.05, 0), -PI / 2], [Vector3(-11, 0.05, 0), PI / 2]]:
		player.release_pointer()
		player.position = edge[0]
		player.rotation.y = edge[1]
		await frames(10)
		await key(KEY_W, true)
		await tap(KEY_SPACE)
		await frames(90)
		await key(KEY_W, false)
		var distance: float = maxf(absf(player.position.x), absf(player.position.z))
		check(distance > 11.5 and distance < 11.65 and player.is_on_floor(), "Walking/jumping escapes or fails to reach boundary: " + str(player.position))
	# Corner containment and a reset while movement is held.
	player.position = Vector3(11, 0.05, -11)
	player.rotation.y = 0
	await frames(10)
	await key(KEY_W, true)
	await key(KEY_D, true)
	await frames(60)
	check(player.position.x < 11.65 and player.position.z > -11.65, "Diagonal walk crosses the corner")
	# Observe reset synchronously before physics settles the 5cm spawn clearance.
	var home := InputEventKey.new()
	home.physical_keycode = KEY_HOME
	home.pressed = true
	player._unhandled_input(home)
	check(player.position.is_equal_approx(player.spawn_position) and is_equal_approx(player.rotation.y, player.spawn_yaw) and is_equal_approx(player.camera.rotation.x, player.spawn_pitch), "Home does not use the configured outdoor spawn")
	check(player._held.is_empty() and player.velocity.is_zero_approx(), "Home retains movement")
	await key(KEY_W, false)
	await key(KEY_D, false)
	await frames(10)
	if DisplayServer.get_name() != "headless":
		# Reuse existing event helpers for one live-panel drag/rotate/cancel and input.
		var original: Transform3D = object.global_transform
		var pointer := point(window)
		await mouse(pointer, true)
		await motion(pointer + Vector2(70, 0))
		await mouse(pointer, true, false, MOUSE_BUTTON_RIGHT)
		await mouse(pointer, true, false, MOUSE_BUTTON_WHEEL_UP)
		check(objects.dragging and object.position.distance_to(original.origin) > 0.1 and not object.basis.is_equal_approx(original.basis), "Outdoor panel drag/rotation failed")
		await tap(KEY_ESCAPE)
		await mouse(pointer, false)
		await mouse(pointer, false, false, MOUSE_BUTTON_RIGHT)
		check(object.global_transform.is_equal_approx(original), "Cancelled outdoor drag loses the original placement")
		await activate(window)
		var marker := output_dir.path_join("typed.txt")
		if FileAccess.file_exists(marker): DirAccess.remove_absolute(marker)
		await type_text("printf 'OUTDOOR_OK\\n' | tee '" + marker + "'\n")
		check(await wait_file("typed.txt") == "OUTDOOR_OK\n", "Outdoor terminal did not accept typing")
		await capture("application.png")
		await return_to_room()
		check(objects.bindings[window] == object and object.entity_id == identity and object.global_transform.is_equal_approx(original), "World return lost the live panel")
		player.position = Vector3(7, 0.05, 8)
		player.rotation.y = 0.65
		await frames(10)
		await capture("ground-and-boundary.png")
	await study.desktop_runtime.shutdown()
	check(not terminal.session.is_running() and not terminal.child_running(), "Outdoor shutdown left the session/client running")
	print("OUTDOOR_STARTUP_OK mode=", DisplayServer.get_name(), " boundaries=4 failures=", failures)
	quit(1 if failures else 0)
