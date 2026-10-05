## Real Terminal + Vim, rendered panels and injected pointer/key events.
extends "res://tests/app_launcher.gd"

var objects: Node3D

func mouse(position: Vector2, pressed: bool, double_click := false, button := MOUSE_BUTTON_LEFT) -> void:
	var event := InputEventMouseButton.new()
	event.position = position
	event.global_position = position
	event.button_index = button
	event.pressed = pressed
	event.double_click = double_click
	Input.parse_input_event(event)
	await process_frame

func motion(position: Vector2) -> void:
	var event := InputEventMouseMotion.new()
	event.position = position
	event.global_position = position
	Input.parse_input_event(event)
	await process_frame

func return_to_room() -> void:
	await key(KEY_CTRL, true)
	await key(KEY_ALT, true)
	await tap(KEY_ESCAPE)
	await key(KEY_ALT, false)
	await key(KEY_CTRL, false)
	await settle()

func point(window: int) -> Vector2:
	return objects.camera.unproject_position(objects.bindings[window].global_position)

func activate(window: int) -> void:
	var position := point(window)
	await mouse(position, true)
	await mouse(position, false)
	check(not app.active and terminal.session.keyboard_focus_handle() == 0, "Single click typed/activated")
	await mouse(position, true, true)
	await mouse(position, false)
	await settle()
	check(app.active and app.focused_handle == window and terminal.handle == window, "Double-click did not focus targeted window")

func run() -> void:
	if DisplayServer.get_name() == "headless":
		push_error("Application object acceptance requires graphical rendering")
		quit(1)
		return
	output_dir = ProjectSettings.globalize_path("res://../.tools/application-objects-test").simplify_path()
	DirAccess.make_dir_recursive_absolute(output_dir)
	var marker := output_dir.path_join("target.txt")
	if FileAccess.file_exists(marker): DirAccess.remove_absolute(marker)
	study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	objects = study.application_objects
	var launcher = study.app_launcher
	var player = study.get_node("Player")
	var deadline := Time.get_ticks_msec() + 15000
	while (objects.bindings.is_empty() or launcher.entries.is_empty()) and Time.get_ticks_msec() < deadline: await process_frame
	root.grab_focus()
	await settle()
	check(not objects.bindings.is_empty() and app.host_focused, "Missing live window/host focus")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	var first: int = terminal.handle
	var first_object: Node3D = objects.bindings[first]
	var identity: String = first_object.entity_id
	var initial: Vector3 = first_object.position
	check(objects.pick(point(first)) == first, "Initial object is not pickable")
	await activate(first)
	await type_text("while true; do date +%T; sleep 0.2; done\n")
	await return_to_room()
	check(objects.bindings[first] == first_object and first_object.position == initial, "Room return duplicated or moved object")
	launcher.open()
	launcher.launch("vim.desktop")
	deadline = Time.get_ticks_msec() + 14000
	while not launcher.pending.is_empty() and Time.get_ticks_msec() < deadline: await process_frame
	check(app.active and terminal.handle != first, "Vim did not launch")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	var second: int = terminal.handle
	await type_text("iLIVE SPATIAL EDITOR\n")
	await return_to_room()
	check(objects.bindings.size() == 2, "Two live windows did not produce two objects")
	var second_object: Node3D = objects.bindings[second]
	var second_position: Vector3 = second_object.position
	check(first_object.texture != second_object.texture, "Windows share the same client pixels")
	check(first_object.entity_id != second_object.entity_id, "Entity identity is not independent")
	var revision: int = first_object.revision
	var pixels: PackedByteArray = first_object.texture.get_image().get_data()
	await create_timer(0.5).timeout
	check(first_object.revision > revision, "Background application preview stopped updating")
	check(first_object.texture.get_image().get_data() != pixels, "Background preview pixels are frozen")
	await capture("two-live-applications.png")
	# Move via actual unhandled events, with no camera or client input ownership.
	var position := point(first)
	await mouse(position, true)
	await motion(position + Vector2(100, 25))
	check(objects.dragging and player.application_mode, "Drag did not capture world input")
	await key(KEY_W, true)
	await key(KEY_W, false)
	await mouse(position + Vector2(100, 25), true, false, MOUSE_BUTTON_WHEEL_UP)
	await mouse(position + Vector2(100, 25), false)
	check(first_object.position.distance_to(initial) > 0.2, "Drag/wheel did not move object")
	check(second_object.position == second_position, "Moving one window moved another")
	check(first_object.entity_id == identity and not player.application_mode and player._held.is_empty(), "Drag changed identity or left held navigation")
	var moved: Vector3 = first_object.position
	await activate(first)
	await chord(KEY_CTRL, KEY_C)
	await type_text("printf TARGET > '" + marker + "'\n")
	check(await wait_file("target.txt") == "TARGET", "Double-click did not route real shell input to its target")
	await return_to_room()
	check(first_object.position == moved and first_object.entity_id == identity, "Activation/return lost placement")
	await capture("moved-applications.png")
	# Escape, launcher and host focus loss cancel an in-progress move.
	for cause in ["escape", "launcher", "focus"]:
		position = point(first)
		await mouse(position, true)
		await motion(position + Vector2(0, -60))
		check(objects.dragging, "Missing cancellation drag")
		if cause == "escape": await tap(KEY_ESCAPE)
		elif cause == "launcher":
			await tap(KEY_TAB)
			check(launcher.opened, "Tab did not open launcher during drag")
			launcher.close()
		else:
			objects._notification(Node.NOTIFICATION_APPLICATION_FOCUS_OUT)
			app._notification(Node.NOTIFICATION_APPLICATION_FOCUS_OUT)
			app._notification(Node.NOTIFICATION_APPLICATION_FOCUS_IN)
		await mouse(position, false)
		check(objects.pressed == 0 and first_object.position == moved and not player.application_mode, "Drag cancellation failed: " + cause)
	await mouse(point(first), true)
	await mouse(launcher.open_button.get_global_rect().get_center(), false)
	check(objects.pressed == 0 and not player.application_mode and not launcher.opened, "GUI swallowed world drag release")
	# Opaque room geometry occludes picks. Clamp the entire panel, not its center.
	var saved: Vector3 = first_object.position
	first_object.position = Vector3(0, 1.6, -5)
	check(objects.pick(point(first)) != first, "Picked application through the back wall")
	first_object.position = objects.bounded_position(first_object, Vector3(100, 100, 100))
	for corner_x in [-0.7, 0.7]:
		for corner_y in [-0.51, 0.51]:
			for corner_z in [-0.04, 0.04]:
				var corner := first_object.to_global(Vector3(corner_x, corner_y, corner_z))
				check(corner.x <= 3.861 and corner.y <= 3.151 and corner.z <= 4.361, "Panel extends outside room")
	first_object.position = saved
	check(not objects.placement_clear(first_object, second_object.position), "Placement overlaps another panel")
	check(not objects.placement_clear(first_object, Vector3(0, -0.08, 0)), "Placement intersects floor")
	second_object.position = first_object.position + objects.camera.global_position.direction_to(first_object.position) * 0.8
	check(objects.pick(point(first)) == first, "Nearest application does not win overlapping picks")
	first_object.rotation.y += PI
	check(objects.pick(point(first)) == 0, "Picked through an opaque panel back")
	first_object.rotation.y -= PI
	second_object.position = second_position
	# A real client exits itself while its world object is being dragged.
	await activate(first)
	await type_text("sleep 1; exit\n")
	await return_to_room()
	position = point(first)
	await mouse(position, true)
	await motion(position + Vector2(0, -30))
	deadline = Time.get_ticks_msec() + 5000
	while objects.bindings.has(first) and Time.get_ticks_msec() < deadline: await process_frame
	check(not objects.bindings.has(first) and objects.bindings.has(second), "Closing one client removed the wrong object")
	check(objects.pressed == 0 and not app.active and not player.application_mode, "Destruction left drag or application focus")
	await mouse(position, false)
	await activate(second)
	await tap(KEY_ESCAPE)
	await type_text(":q!\n")
	deadline = Time.get_ticks_msec() + 5000
	while not objects.bindings.is_empty() and Time.get_ticks_msec() < deadline: await process_frame
	check(objects.bindings.is_empty(), "Vim no longer accepts input, or stale objects remain")
	await terminal.shutdown()
	check(terminal.launched_children.is_empty(), "Owned clients survived shutdown")
	print("APPLICATION_OBJECTS_OK clients=2 failures=", failures)
	quit(1 if failures else 0)
