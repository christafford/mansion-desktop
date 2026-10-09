## GPU acceptance of rotated-room assemblies, scaled props and live supports.
extends "res://tests/furniture_drag.gd"

func run() -> void:
	if DisplayServer.get_name() == "headless":
		push_error("Room furniture acceptance requires graphical rendering")
		quit(1)
		return
	output_dir = ProjectSettings.globalize_path("res://../.tools/room-furniture-test")
	DirAccess.make_dir_recursive_absolute(output_dir)
	study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	objects = study.application_objects
	var player = study.get_node("Player")
	var spaces = study.get_node("Hallway/Rooms")
	player.set_physics_process(false)
	var deadline := Time.get_ticks_msec() + 15000
	while objects.bindings.is_empty() and Time.get_ticks_msec() < deadline: await process_frame
	root.grab_focus()
	await settle()
	check(not objects.bindings.is_empty() and app.host_focused, "Missing live client/host focus")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	check(get_nodes_in_group("antique_bookcases").size() == 8, "Missing antique bookcase bays")
	check(spaces.rooms[0].get_node("AntiqueBookcase0/Books").mesh.get_surface_count() == 2, "Book set is not batched by its two original materials")
	for body in get_nodes_in_group("room_furniture"):
		check(body is RigidBody3D and body.is_in_group("pushable_furniture"), "Furniture bypasses shared controls")
		check(body.find_children("*", "StaticBody3D", true, false).is_empty(), "Moving assembly contains a static collider: " + str(body.get_path()))
		check(body.scale.is_equal_approx(Vector3.ONE) and body.placement_box.has_volume(), "Invalid rigid body scale/bounds")
	var plant: RigidBody3D
	for body in get_nodes_in_group("room_furniture"):
		if body.kind == "potted_plant_02" and is_equal_approx(body.model_scale, 1.35): plant = body
	check(plant != null, "Missing scaled garden plant")
	if plant != null:
		var pot = plant.find_children("*", "CollisionShape3D", true, false)[0]
		check(is_equal_approx(pot.shape.radius, 0.232 * 1.35) and is_equal_approx(pot.shape.height, 0.336 * 1.35), "Scaled plant collider does not fit pot")
	var candidates := [spaces.rooms[0].get_node("AntiqueBookcase0"), plant,
		spaces.rooms[0].get_node("dining_chair_02"), spaces.rooms[0].get_node("Worktable"),
		spaces.rooms[1].get_node("MapCabinet"), spaces.rooms[1].get_node("Armillary"),
		spaces.rooms[3].get_node("DisplayStand"), spaces.rooms[4].get_node("PartsBin")]
	# Keep the original rotated parent while exercising each type in a clear area.
	for body in candidates:
		if body == null: continue
		var original: Transform3D = body.global_transform
		var label: String = body.name
		player.position = Vector3(-3.8, 0.05, 20)
		await place(body, Transform3D(Basis.IDENTITY, Vector3(-2.3, 0.1, 21)))
		for frame in range(40): await physics_frame
		player.position = Vector3(-2.3, 0.05, 23)
		player.rotation = Vector3.ZERO
		player.camera.position = Vector3(0, 1.6, 0)
		player.camera.look_at(body.to_global(body.placement_box.get_center()))
		await settle()
		var pointer: Vector2 = objects.camera.unproject_position(body.to_global(body.placement_box.get_center()))
		check(objects.pick_furniture(pointer) == body, label + " not pickable")
		var before: Transform3D = body.global_transform
		var content: MeshInstance3D = body.find_children("*", "MeshInstance3D", true, false)[-1]
		var content_pose: Transform3D = body.global_transform.affine_inverse() * content.global_transform
		await mouse(pointer, true)
		check(objects.furniture == body and body.freeze, label + " not grabbed")
		await motion(pointer + Vector2(70, -70))
		check(body.global_position.distance_to(before.origin) > 0.15, label + " not moved")
		var moved: Vector3 = body.global_position
		player.set_physics_process(true)
		await key(KEY_W, true)
		for frame in range(8): await physics_frame
		await key(KEY_W, false)
		player.set_physics_process(false)
		check(body.global_position.distance_to(moved) > 0.15, label + " not carried while walking")
		var rotation_before: Basis = body.global_basis
		await mouse(pointer, true, false, MOUSE_BUTTON_RIGHT)
		await mouse(pointer, true, false, MOUSE_BUTTON_WHEEL_UP)
		await mouse(pointer, false, false, MOUSE_BUTTON_RIGHT)
		check(body.global_basis.get_rotation_quaternion().angle_to(rotation_before.get_rotation_quaternion()) > 0.1, label + " not rotated: " + str(body.global_transform))
		check((body.global_transform.affine_inverse() * content.global_transform).is_equal_approx(content_pose), label + " left contents behind")
		await tap(KEY_ESCAPE)
		await mouse(pointer, false)
		check(body.global_position.distance_to(before.origin) < 0.025 and body.global_basis.get_rotation_quaternion().angle_to(before.basis.get_rotation_quaternion()) < 0.02 and not body.freeze, label + " cancel lost global pose")
		pointer = objects.camera.unproject_position(body.to_global(body.placement_box.get_center()))
		await mouse(pointer, true)
		await motion(pointer + Vector2(0, -85))
		check(body.global_position.y > before.origin.y + 0.08, label + " cannot lift: pose=%s grab=%s" % [body.global_transform, objects.furniture])
		await mouse(pointer, false)
		for frame in range(65): await physics_frame
		check(not objects.has_grab() and not body.freeze and absf(body.global_position.y) < 0.04, label + " did not settle on floor")
		player.position = body.global_position + Vector3(0, 0.05, 1.5)
		player.rotation = Vector3.ZERO
		player.camera.rotation = Vector3(-0.15, 0, 0)
		var start: Vector3 = body.global_position
		player.set_physics_process(true)
		await key(KEY_W, true)
		for frame in range(55): await physics_frame
		await key(KEY_W, false)
		player.set_physics_process(false)
		check(body.global_position.distance_to(start) > 0.12, label + " does not yield to walking")
		check(absf(body.global_basis.y.dot(Vector3.UP) - 1) < 0.01, label + " tipped over")
		player.position = Vector3(-3.8, 0.05, 20)
		await place(body, original)
	# Pick a bookcase from its authored row and pull it into the library aisle.
	var shelf = candidates[0]
	var shelf_pose: Transform3D = shelf.global_transform
	player.position = spaces.rooms[0].to_global(Vector3(0.3, 0.05, 2.4))
	player.rotation = Vector3(0, spaces.ROOMS[0].yaw, 0)
	player.camera.look_at(shelf.to_global(shelf.placement_box.get_center()))
	await capture("antique-bookcase.png")
	var shelf_pointer: Vector2 = objects.camera.unproject_position(shelf.to_global(shelf.placement_box.get_center()))
	await mouse(shelf_pointer, true)
	check(objects.furniture == shelf, "Cannot select bookcase from its library row")
	await mouse(shelf_pointer, true, false, MOUSE_BUTTON_WHEEL_DOWN, 2.0)
	check(shelf.global_position.distance_to(shelf_pose.origin) > 0.3, "Cannot pull bookcase out of its library row")
	await capture("antique-bookcase-carried.png")
	await tap(KEY_ESCAPE)
	await mouse(shelf_pointer, false)
	# Carry a real application through the newly added half of each longer room.
	var window: int = terminal.handle
	var panel = objects.bindings[window]
	var identity: String = panel.entity_id
	for i in range(6):
		var room: Node3D = spaces.rooms[i]
		check(is_equal_approx(room.get_meta("depth"), 12.8 if i < 4 else (6.4 if i == 4 else 24.0)), "Incorrect room depth")
		if i >= 4: continue
		player.position = room.to_global(Vector3(0, 0.05, 6.5))
		player.rotation = Vector3(0, spaces.ROOMS[i].yaw + PI, 0)
		player.camera.rotation = Vector3.ZERO
		panel.gravity_active = false
		panel.fall_speed = 0
		panel.position = room.to_global(Vector3(0, 1.6, 7.8))
		panel.rotation = Vector3(0, spaces.ROOMS[i].yaw + PI, 0)
		await settle()
		await mouse(point(window), true)
		check(objects.pressed == window, "Cannot grab in extended " + str(room.name))
		player.set_physics_process(true)
		await key(KEY_W, true)
		for frame in range(55): await physics_frame
		await key(KEY_W, false)
		player.set_physics_process(false)
		check(room.to_local(player.position).z > 8.5 and room.to_local(panel.position).z > 9.8, "Cannot traverse/carry in extended " + str(room.name))
		check(player.position.y > -0.01 and panel.entity_id == identity, "Extended floor/identity lost")
		await mouse(point(window), false)
		for frame in range(60): await physics_frame
		check(panel.position.y >= panel.bounds.y / 2 - 0.03, "Panel fell through extended floor")
		await capture(str(room.name) + "-extended.png")
	# A moved room table must stop supporting a live application.
	var table = spaces.rooms[0].get_node("Worktable")
	player.position = Vector3(-3.8, 0.05, 20)
	await place(table, Transform3D(Basis.IDENTITY, Vector3(-2.3, 0.1, 21)))
	for frame in range(40): await physics_frame
	panel.position = Vector3(-2.9, 2.4, 21)
	panel.rotation = Vector3.ZERO
	panel.gravity_active = true
	panel.fall_speed = 0
	for frame in range(90): await physics_frame
	check(panel.support != null and panel.support.get_ref() == table, "Live panel did not land on movable table")
	check(absf(panel.position.y - panel.bounds.y / 2 - 0.885) < 0.04, "Table support height incorrect")
	player.position = Vector3(-2.3, 0.05, 23.5)
	player.rotation = Vector3.ZERO
	player.camera.look_at(table.to_global(Vector3(0.65, 0.65, 0)))
	await capture("live-table-support.png")
	var pointer: Vector2 = objects.camera.unproject_position(table.to_global(Vector3(0.65, 0.65, 0)))
	await mouse(pointer, true)
	check(objects.furniture == table, "Cannot grab supporting table")
	await motion(pointer + Vector2(600, 0))
	await mouse(pointer, false)
	for frame in range(90): await physics_frame
	check(panel.position.y - panel.bounds.y / 2 < 0.04, "Removed table leaves application floating")
	player.camera.look_at(panel.position)
	await activate(window)
	var marker := output_dir.path_join("typed.txt")
	if FileAccess.file_exists(marker): DirAccess.remove_absolute(marker)
	await type_text("printf 'MOVABLE' > '" + marker.replace("'", "'\"'\"'") + "'\n")
	await settle()
	check(FileAccess.file_exists(marker) and FileAccess.get_file_as_string(marker) == "MOVABLE", "Moved/fallen application lost live typing")
	await return_to_room()
	await capture("removed-table-support.png")
	await terminal.shutdown()
	print("ROOM_FURNITURE_OK types=8 extended=4 failures=", failures)
	quit(1 if failures else 0)
