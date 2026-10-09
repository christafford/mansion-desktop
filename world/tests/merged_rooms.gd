## Removed partitions must be open in rendering, physics and carry bounds.
extends "res://tests/hallway.gd"

func run() -> void:
	output_dir = ProjectSettings.globalize_path("res://../.tools/merged-rooms-test")
	DirAccess.make_dir_recursive_absolute(output_dir)
	study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	objects = study.application_objects
	var player = study.get_node("Player")
	var hall = study.get_node("Hallway")
	var deadline := Time.get_ticks_msec() + 15000
	while objects.bindings.is_empty() and Time.get_ticks_msec() < deadline: await process_frame
	root.grab_focus()
	await settle()
	check(not objects.bindings.is_empty() and app.host_focused, "Missing real client/host focus")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	check(not hall.has_node("LeftDoor7") and not hall.has_node("RightDoor13"), "Removed entrance still exists")
	check(hall.has_node("LeftDoor11") and hall.has_node("RightDoor9") and hall.has_node("LeftDoor15"), "Wrong side entrances removed")
	for removed in [Vector3(1, 0, 7), Vector3(-1, 0, 13)]:
		await walk(player, Vector3(0, 0.05, removed.z), -signf(removed.x) * PI / 2, 50)
		check(absf(player.position.x) < 1.13, "Former entrance was not sealed")
	var window: int = terminal.handle
	var panel = objects.bindings[window]
	var identity: String = panel.entity_id
	for side in [1, -1]:
		var center: float = 9 if side == 1 else 11
		for depth in [2.0, 7.0, 10.5]:
			for direction in [1, -1]:
				player.release_pointer()
				player.position = Vector3(side * (1.4 + depth), 0.05, center - direction * 1.4)
				player.rotation = Vector3(0, PI if direction == 1 else 0, 0)
				player.camera.rotation = Vector3.ZERO
				panel.gravity_active = false
				panel.fall_speed = 0
				panel.position = player.position + Vector3(0, 1.55, direction * 1.1)
				panel.rotation = Vector3(0, player.rotation.y, 0)
				await settle()
				await mouse(point(window), true)
				check(objects.pressed == window, "Cannot grab before partition crossing")
				await key(KEY_W, true)
				for frame in range(70): await physics_frame
				await key(KEY_W, false)
				check(direction * (player.position.z - center) > 1.0, "Former partition blocks walking: " + str(player.position))
				check(direction * (panel.position.z - center) > 2.0, "Former partition clamps carried panel: " + str(panel.position))
				check(absf(player.position.y) < 0.04 and panel.entity_id == identity, "Merged floor/identity lost")
				await mouse(point(window), false)
				for frame in range(55): await physics_frame
				check(panel.position.y >= panel.bounds.y / 2 - 0.03, "Panel fell through merged floor")
		player.set_physics_process(false)
		player.camera.global_position = Vector3(side * 3.0, 1.8, center - 2.6)
		player.camera.look_at(Vector3(side * 9.0, 1.35, center + 0.5))
		await capture("left-wing.png" if side == 1 else "right-wing.png")
		player.camera.position = Vector3(0, 1.6, 0)
		player.set_physics_process(true)
	# Use the real client after the last crossing.
	player.set_physics_process(false)
	player.camera.global_position = panel.position + Vector3(0, 0.7, 2.2)
	player.camera.look_at(panel.position)
	panel.rotation.y = 0
	await settle()
	await activate(window)
	var marker := output_dir.path_join("typed.txt")
	if FileAccess.file_exists(marker): DirAccess.remove_absolute(marker)
	await type_text("printf 'WINGS' > '" + marker.replace("'", "'\"'\"'") + "'\n")
	await settle()
	check(FileAccess.file_exists(marker) and FileAccess.get_file_as_string(marker) == "WINGS", "Merged-room client lost typing")
	await return_to_room()
	await terminal.shutdown()
	print("MERGED_ROOMS_OK crossings=12 sealed=2 failures=", failures)
	quit(1 if failures else 0)
