## Agent-rendered gallery and actual controller/live-panel threshold checks.
extends "res://tests/application_objects.gd"

func walk(player: Node, position: Vector3, yaw: float, frames: int, fast := false) -> void:
	player.release_pointer()
	player.position = position
	player.rotation = Vector3(0, yaw, 0)
	player.camera.rotation = Vector3.ZERO
	for frame in range(3): await physics_frame
	if fast: await key(KEY_SHIFT, true)
	await key(KEY_W, true)
	for frame in range(frames): await physics_frame
	await key(KEY_W, false)
	if fast: await key(KEY_SHIFT, false)

func run() -> void:
	output_dir = ProjectSettings.globalize_path("res://../.tools/hallway-test")
	DirAccess.make_dir_recursive_absolute(output_dir)
	study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	objects = study.application_objects
	var player = study.get_node("Player")
	var hall = study.get_node("Hallway")
	# Hold doors closed for this collision regression; mansion_rooms.gd tests opening.
	for door in get_nodes_in_group("hallway_doors"): door.get_node("Hinge").set_physics_process(false)
	var deadline := Time.get_ticks_msec() + 15000
	while objects.bindings.is_empty() and Time.get_ticks_msec() < deadline: await process_frame
	root.grab_focus()
	await settle()
	check(not objects.bindings.is_empty() and app.host_focused, "Missing live client/host focus")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	var doors := get_nodes_in_group("hallway_doors")
	check(doors.size() == 6, "Expected six closed doors")
	var left := 0
	var right := 0
	for door in doors:
		if door.position.x > 0.5: left += 1
		elif door.position.x < -0.5: right += 1
	check(left == 3 and right == 2 and hall.has_node("EndDoor"), "Door distribution differs from request")
	check(hall.lights.size() == 6, "Missing oil lamps")
	for light in hall.lights: check(light.shadow_enabled, "Oil lamp lacks shadows")
	player.set_physics_process(false)
	hall.set_process(false)
	var views := [
		["entrance", Vector3(0, 1.65, 1.5), Vector3(0, 1.55, 14)],
		["gallery", Vector3(0, 1.65, 5.6), Vector3(0, 1.55, 18.4)],
		["return", Vector3(0, 1.65, 17.2), Vector3(0, 1.5, 0)],
		["oil-lamp", Vector3(0.45, 1.77, 8.3), Vector3(1.0, 1.65, 9)],
		["door", Vector3(-0.2, 1.65, 6.2), Vector3(1.3, 1.25, 7)]
	]
	for spec in views:
		player.camera.global_position = spec[1]
		player.camera.look_at(spec[2])
		await capture(spec[0] + ".png")
	if "--hallway-captures-only" in OS.get_cmdline_user_args():
		await terminal.shutdown()
		print("HALLWAY_RENDERED failures=", failures)
		quit(1 if failures else 0)
		return
	hall.set_process(true)
	player.camera.position = Vector3(0, 1.6, 0)
	player.set_physics_process(true)
	await walk(player, Vector3(0, 0.05, 2.8), PI, 250, true)
	check(player.position.z > 17.8 and player.position.z < 18.2, "Open threshold/full hallway/end door collision failed: " + str(player.position))
	check(absf(player.position.y) < 0.04, "Hall floor is not continuous")
	await walk(player, player.position, 0, 232, true)
	check(player.position.z < 3.2 and player.position.z > 2.2, "Cannot walk back through the opening: " + str(player.position))
	# Check every side door and a stretch of wall between doors.
	for door in doors:
		if absf(door.position.x) < 0.5: continue
		await walk(player, Vector3(0, 0.05, door.position.z), -signf(door.position.x) * PI / 2, 45)
		check(absf(player.position.x) > 0.8 and absf(player.position.x) < 1.1, "Closed door does not block the player: " + str(door.name))
	await walk(player, Vector3(0, 0.05, 6), -PI / 2, 45)
	check(player.position.x < 1.13, "Player escaped through hallway wall")
	# Carry the actual live terminal through the doorway and drop it in the hall.
	player.release_pointer()
	player.position = Vector3(0, 0.05, 2.5)
	player.rotation = Vector3(0, PI, 0)
	player.camera.rotation = Vector3.ZERO
	var window: int = terminal.handle
	var panel = objects.bindings[window]
	panel.position = Vector3(0, 1.65, 3.95)
	panel.rotation = Vector3(0, PI, 0)
	await settle()
	await mouse(point(window), true)
	check(objects.pressed == window, "Cannot grab terminal facing doorway")
	await key(KEY_W, true)
	for frame in range(140): await physics_frame
	await key(KEY_W, false)
	check(player.position.z > 7.5 and panel.position.z > 9, "Carried panel stuck at old study bounds: player=%s panel=%s" % [player.position, panel.position])
	await mouse(point(window), false)
	for frame in range(90): await physics_frame
	check(panel.position.z > 9 and absf(panel.position.y - panel.bounds.y / 2) < 0.035, "Panel did not land in hallway")
	player.camera.look_at(panel.position)
	await activate(window)
	check(app.active and app.focused_handle == window, "Hallway application lost focus/activation")
	await return_to_room()
	# Full rotated bounds remain inside the expanded mansion, not just its center.
	panel.rotation.y = 0.4
	panel.position = objects.bounded_position(panel, Vector3(100, 100, 100))
	for x in [-0.7, 0.7]:
		for y in [-0.51, 0.51]:
			for z in [-0.04, 0.04]:
				var corner: Vector3 = panel.to_global(Vector3(x, y, z))
				check(absf(corner.x) <= 4.361 and corner.y <= 3.081 and corner.z <= 26.361, "Panel extends outside mansion")
	await terminal.shutdown()
	print("HALLWAY_OK doors=6 lamps=6 failures=", failures)
	quit(1 if failures else 0)
