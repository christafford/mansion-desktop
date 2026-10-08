## GPU room views, real walking/carrying, real clients and session-local locations.
extends "res://tests/hallway.gd"

func profile_view(label: String) -> void:
	var samples: Array[float] = []
	var before := Time.get_ticks_usec()
	for frame in range(120):
		await process_frame
		var now := Time.get_ticks_usec()
		samples.append((now - before) / 1000.0)
		before = now
	samples.sort()
	print("ROOM_FRAME_MS ", label, " median=", samples[60], " p95=", samples[114], " max=", samples[119], " static_bytes=", OS.get_static_memory_usage())

func run() -> void:
	if DisplayServer.get_name() == "headless":
		push_error("Room acceptance requires a graphical renderer")
		quit(1)
		return
	output_dir = ProjectSettings.globalize_path("res://../.tools/mansion-rooms-test")
	DirAccess.make_dir_recursive_absolute(output_dir)
	study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	objects = study.application_objects
	var player = study.get_node("Player")
	var spaces = study.get_node("Hallway/Rooms")
	var specs = spaces.ROOMS
	var deadline := Time.get_ticks_msec() + 15000
	while objects.bindings.is_empty() and Time.get_ticks_msec() < deadline: await process_frame
	root.grab_focus()
	await settle()
	check(not objects.bindings.is_empty() and app.host_focused, "Missing live client/host focus")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	check(get_nodes_in_group("mansion_rooms").size() == 6, "Six distinct destinations missing")
	check(get_nodes_in_group("room_work_surfaces").size() == 7, "A room lacks work surfaces")
	var ids := {}
	for room in spaces.rooms:
		check(not ids.has(room.get_meta("room_id")), "Room IDs collide")
		ids[room.get_meta("room_id")] = true
	for door in get_nodes_in_group("hallway_doors"):
		check(not door.get_node("Hinge").opened, "Distant door opened without approach")
	player.set_physics_process(false)
	for room in spaces.rooms:
		player.camera.global_position = room.to_global(Vector3(0.65 if room.name != "observatory" else 2.5, 1.7, 0.7))
		player.camera.look_at(room.to_global(Vector3(0, 1.35, 4.8)))
		await capture(str(room.name) + ".png")
		if "--rooms-profile" in OS.get_cmdline_user_args(): await profile_view(str(room.name))
	player.camera.global_position = Vector3(0, 1.65, 5.7)
	player.camera.look_at(Vector3(0, 1.6, 18.5))
	await capture("hall-entrances.png")
	if "--rooms-captures-only" in OS.get_cmdline_user_args() or "--rooms-profile" in OS.get_cmdline_user_args():
		await terminal.shutdown()
		print("ROOMS_RENDERED failures=", failures)
		quit(1 if failures else 0)
		return
	player.camera.position = Vector3(0, 1.6, 0)
	player.camera.rotation = Vector3.ZERO
	player.set_physics_process(true)
	var marker := output_dir.path_join("typed.txt")
	if FileAccess.file_exists(marker): check(DirAccess.remove_absolute(marker) == OK, "Cannot clear typing marker")
	var window: int = terminal.handle
	var panel = objects.bindings[window]
	var identity: String = panel.entity_id
	var door_names := ["LeftDoor7", "RightDoor9", "LeftDoor11", "RightDoor13", "LeftDoor15", "EndDoor"]
	for i in range(specs.size()):
		var room: Node3D = spaces.rooms[i]
		var spec: Dictionary = specs[i]
		player.release_pointer()
		player.position = room.to_global(Vector3(0, 0.05, -2.0))
		player.rotation = Vector3(0, spec.yaw + PI, 0)
		player.camera.rotation = Vector3.ZERO
		panel.gravity_active = false
		panel.fall_speed = 0.0
		panel.position = room.to_global(Vector3(0, 1.6, -0.6))
		panel.rotation = Vector3(0, spec.yaw + PI, 0)
		for frame in range(32): await physics_frame
		var hinge = study.get_node("Hallway/" + door_names[i] + "/Hinge")
		check(hinge.opened and hinge.progress == 1.0, "Approach did not open " + spec.id)
		await mouse(point(window), true)
		check(objects.pressed == window, "Cannot grab before " + spec.id)
		await key(KEY_W, true)
		for frame in range(105): await physics_frame
		await key(KEY_W, false)
		check(room.to_local(player.position).z > 1.9, "Player blocked at " + spec.id + ": " + str(room.to_local(player.position)))
		check(room.to_local(panel.position).z > 3.2, "Panel stuck at " + spec.id + ": " + str(room.to_local(panel.position)))
		await mouse(point(window), false)
		for frame in range(65): await physics_frame
		check(panel.position.y >= panel.bounds.y / 2 - 0.03 and panel.position.y < 1.5, "Panel fell through room/support " + spec.id)
		check(panel.entity_id == identity, "Carrying changed identity")
		# Walk back along the same clear center route; doors remain open.
		await walk(player, room.to_global(Vector3(0, 0.05, 2.0)), spec.yaw, 95)
		check(room.to_local(player.position).z < -1.5, "Cannot leave " + spec.id)
		check(hinge.progress == 1.0, "Door closed on return")
		if i == 0:
			player.camera.look_at(room.to_global(Vector3(0, 1.5, 3)))
			await capture("open-library-door.png")
			player.camera.rotation = Vector3.ZERO
		# Solid side boundary, away from the worktable.
		await walk(player, room.to_global(Vector3(0, 0.05, 1.0)), spec.yaw - PI / 2, 145, true)
		check(room.to_local(player.position).x < spec.width / 2 - 0.2, "Escaped side wall " + spec.id)
	# Move the real panel onto the observatory worktable and exercise focus/return.
	var observatory: Node3D = spaces.rooms[5]
	panel.gravity_active = true
	panel.position = observatory.to_global(Vector3(-2.9, 2.3, 5.7))
	panel.rotation.y = PI
	for frame in range(90): await physics_frame
	check(absf(panel.position.y - (0.885 + panel.bounds.y / 2)) < 0.04, "Panel did not land on observatory table: " + str(panel.position))
	player.position = observatory.to_global(Vector3(-2.9, 0.05, 3.4))
	player.rotation.y = PI
	player.camera.rotation = Vector3.ZERO
	await settle()
	await activate(window)
	await type_text("printf 'OBSERVATORY WORKSPACE\\n'; printf 'OBSERVATORY' > '" + marker.replace("'", "'\"'\"'") + "'\n")
	await settle()
	check(FileAccess.file_exists(marker) and FileAccess.get_file_as_string(marker) == "OBSERVATORY", "Real shell did not receive observatory typing")
	await return_to_room()
	var placed: Vector3 = panel.position
	await capture("observatory-application.png")
	# Launch another real instance in the library. It must appear here while the
	# observatory's application keeps its identity and physical location.
	var library: Node3D = spaces.rooms[0]
	player.position = library.to_global(Vector3(0, 0.05, 1.3))
	player.rotation.y = specs[0].yaw + PI
	player.camera.rotation = Vector3.ZERO
	await settle()
	var launcher = study.app_launcher
	launcher.open()
	launcher.launch(launcher.TERMINAL_ID)
	deadline = Time.get_ticks_msec() + 15000
	while not launcher.pending.is_empty() and Time.get_ticks_msec() < deadline: await process_frame
	check(app.active and terminal.handle != window, "New room terminal did not launch independently")
	if app.active:
		await type_text("printf 'LIBRARY WORKSPACE\\n'; printf 'LIBRARY' >> '" + marker.replace("'", "'\"'\"'") + "'\n")
		await settle()
		check(FileAccess.file_exists(marker) and FileAccess.get_file_as_string(marker) == "OBSERVATORYLIBRARY", "Independent library shell did not receive typing")
		await return_to_room()
		var second = objects.bindings.get(terminal.handle)
		check(second != null and library.to_local(second.position).z > 0 and absf(library.to_local(second.position).x) < 1.8, "New application spawned outside library")
		check(panel.position.distance_to(placed) < 0.02 and panel.entity_id == identity, "Other room's application moved or changed identity")
		await capture("library-application.png")
	await terminal.shutdown()
	print("MANSION_ROOMS_OK rooms=6 failures=", failures)
	quit(1 if failures else 0)
