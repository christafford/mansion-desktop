## Real Terminal + Vim, rendered panels and injected pointer/key events.
extends "res://tests/app_launcher.gd"

var objects: Node3D

func mouse(position: Vector2, pressed: bool, double_click := false, button := MOUSE_BUTTON_LEFT, factor := 1.0) -> void:
	var event := InputEventMouseButton.new()
	event.position = position
	event.global_position = position
	event.button_index = button
	event.pressed = pressed
	event.double_click = double_click
	event.factor = factor
	Input.parse_input_event(event)
	# Calls from physics_frame can precede dispatch of buffered input next frame.
	await process_frame
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
	study = load("res://scenes/study.tscn").instantiate()
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
	# Carry a real live terminal while walking and looking, without mouse dragging.
	await key(KEY_W, true)
	await mouse(point(first), true)
	check(player._held.has(KEY_W), "Picking up an application stops held walking")
	var carry_start: Vector3 = player.position
	var carried_start: Vector3 = first_object.position
	var grabbed_local: Vector3 = objects.camera.to_local(first_object.global_position)
	for frame in range(12): await physics_frame
	check(player.position.distance_to(carry_start) > 0.3, "Cannot walk while holding an application")
	check(first_object.position.distance_to(carried_start) > 0.3, "Held application stays behind when walking")
	check(objects.camera.to_local(first_object.global_position).distance_to(grabbed_local) < 0.06, "Walking changes the view-relative grab")
	await mouse(point(first), true, false, MOUSE_BUTTON_RIGHT)
	var carry_yaw: float = player.rotation.y
	var look := InputEventMouseMotion.new()
	look.screen_relative = Vector2(40, -20)
	Input.parse_input_event(look)
	for frame in range(3): await physics_frame
	check(player.rotation.y < carry_yaw - 0.05 and player._look_held, "Cannot look while holding an application")
	check(objects.camera.to_local(first_object.global_position).distance_to(grabbed_local) < 0.06, "Looking loses the view-relative grab")
	check(not app.active and terminal.session.keyboard_focus_handle() == 0, "Carrying sends movement keys to the client")
	await mouse(point(first), false)
	check(objects.pressed == 0 and player._held.has(KEY_W) and player._look_held, "Dropping interrupts held walking or looking: pressed=%s keys=%s look=%s" % [objects.pressed, player._held, player._look_held])
	carry_start = player.position
	var dropped_position: Vector3 = first_object.position
	for frame in range(8): await physics_frame
	check(player.position.distance_to(carry_start) > 0.2, "Walking stops after dropping")
	check(Vector2(first_object.position.x, first_object.position.z).distance_to(Vector2(dropped_position.x, dropped_position.z)) < 0.01, "Dropped application follows the camera")
	check(first_object.position.y < dropped_position.y - 0.04, "Released moved application does not fall")
	await key(KEY_W, false)
	await mouse(point(first), false, false, MOUSE_BUTTON_RIGHT)
	# Restore the fixture's starting poses for the two-client placement checks.
	player.return_to_spawn()
	first_object.gravity_active = false
	first_object.fall_speed = 0.0
	first_object.position = initial
	await settle()
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
	# Move via actual unhandled events; walking remains available without client input.
	var position := point(first)
	await mouse(position, true)
	await motion(position + Vector2(100, 25))
	check(objects.dragging and not player.application_mode, "Drag blocks world navigation")
	await key(KEY_W, true)
	await key(KEY_W, false)
	var depth_before: float = -objects.camera.to_local(first_object.global_position).z
	await mouse(position + Vector2(100, 25), true, false, MOUSE_BUTTON_WHEEL_UP)
	var depth_farther: float = -objects.camera.to_local(first_object.global_position).z
	check(depth_farther > depth_before + 0.2, "Wheel up does not push application farther away")
	await mouse(position + Vector2(100, 25), true, false, MOUSE_BUTTON_WHEEL_DOWN)
	var depth_nearer: float = -objects.camera.to_local(first_object.global_position).z
	check(depth_nearer < depth_farther - 0.2 and absf(depth_nearer - depth_before) < 0.01, "Wheel down does not bring application nearer")
	await mouse(position + Vector2(100, 25), true, false, MOUSE_BUTTON_WHEEL_UP, 0.5)
	check(absf(-objects.camera.to_local(first_object.global_position).z - depth_before - 0.125) < 0.01, "Fractional wheel step lost its magnitude")
	# Both buttons + wheel turn only the held application, never camera or depth.
	await mouse(position, true, false, MOUSE_BUTTON_RIGHT)
	var rotation_start: float = first_object.rotation.y
	var rotation_center: Vector3 = first_object.global_position
	var rotation_depth: float = objects.drag_depth
	var camera_basis: Basis = objects.camera.global_basis
	var other_transform: Transform3D = second_object.global_transform
	await mouse(position, true, false, MOUSE_BUTTON_WHEEL_UP)
	check(is_equal_approx(first_object.rotation.y, rotation_start - deg_to_rad(10.0)), "Both buttons + wheel up does not turn panel left")
	await mouse(position, true, false, MOUSE_BUTTON_WHEEL_DOWN)
	check(is_equal_approx(first_object.rotation.y, rotation_start), "Wheel down does not undo panel rotation")
	await mouse(position, true, false, MOUSE_BUTTON_WHEEL_UP, 0.5)
	check(is_equal_approx(first_object.rotation.y, rotation_start - deg_to_rad(5.0)), "Rotation loses fractional wheel steps")
	var half_turn: float = first_object.rotation.y
	await mouse(position, false, false, MOUSE_BUTTON_WHEEL_UP)
	check(is_equal_approx(first_object.rotation.y, half_turn), "Wheel release rotates twice")
	await mouse(position, true, false, MOUSE_BUTTON_WHEEL_UP, 2.5)
	check(is_equal_approx(first_object.rotation.y, rotation_start - deg_to_rad(30.0)), "Repeated rotation did not accumulate")
	check(first_object.global_position.is_equal_approx(rotation_center) and is_equal_approx(objects.drag_depth, rotation_depth), "Rotation changes center or drag depth")
	check(objects.camera.global_basis.is_equal_approx(camera_basis), "Panel rotation rotates the camera")
	check(second_object.global_transform.is_equal_approx(other_transform), "Panel rotation changes another application")
	check(terminal.session.keyboard_focus_handle() == 0 and not app.active, "Panel rotation activates the client")
	await capture("rotating-application.png")
	await mouse(position, false, false, MOUSE_BUTTON_RIGHT)
	var rotated: Vector3 = first_object.rotation
	await mouse(position, true, false, MOUSE_BUTTON_WHEEL_DOWN, 0.5)
	check(is_equal_approx(objects.drag_depth, rotation_depth - 0.125) and first_object.rotation.is_equal_approx(rotated), "Right release does not restore wheel depth adjustment: held=%s mode=%s depth=%s expected=%s rotation=%s expected_rotation=%s" % [objects.pressed, Input.mouse_mode, objects.drag_depth, rotation_depth - 0.125, first_object.rotation, rotated])
	await mouse(position + Vector2(100, 25), false)
	check(first_object.rotation.is_equal_approx(rotated), "Dropping resets rotation")
	check(first_object.position.distance_to(initial) > 0.2, "Drag/wheel did not move object")
	check(second_object.position == second_position, "Moving one window moved another")
	check(first_object.entity_id == identity and not player.application_mode and player._held.is_empty(), "Drag changed identity or left held navigation")
	# Allow the released panel to reach a support before testing saved placement.
	for frame in range(90): await physics_frame
	check(first_object.support != null and first_object.fall_speed == 0, "Dropped application does not settle")
	var moved: Vector3 = first_object.position
	await activate(first)
	await chord(KEY_CTRL, KEY_C)
	await type_text("printf TARGET > '" + marker + "'\n")
	check(await wait_file("target.txt") == "TARGET", "Double-click did not route real shell input to its target")
	await return_to_room()
	check(first_object.position == moved and first_object.rotation.is_equal_approx(rotated) and first_object.entity_id == identity, "Activation/return lost placement or rotation")
	await capture("moved-applications.png")
	# Escape, launcher and host focus loss cancel an in-progress move.
	for cause in ["escape", "launcher", "focus"]:
		position = point(first)
		await mouse(position, true)
		await motion(position + Vector2(0, -60))
		check(objects.dragging, "Missing cancellation drag")
		await mouse(position, true, false, MOUSE_BUTTON_RIGHT)
		await mouse(position, true, false, MOUSE_BUTTON_WHEEL_UP)
		check(not first_object.rotation.is_equal_approx(rotated), "Missing cancellation rotation")
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
		await mouse(position, false, false, MOUSE_BUTTON_RIGHT)
		check(objects.pressed == 0 and first_object.position == moved and first_object.rotation.is_equal_approx(rotated) and not player.application_mode, "Drag/rotation cancellation failed: " + cause)
	await mouse(point(first), true)
	await mouse(launcher.open_button.get_global_rect().get_center(), false)
	check(objects.pressed == 0 and not player.application_mode and not launcher.opened, "GUI swallowed world drag release")
	# Opaque room geometry occludes picks. Clamp the entire panel, not its center.
	var saved: Vector3 = first_object.position
	first_object.position = Vector3(0, 1.6, -5)
	check(objects.pick(point(first)) != first, "Picked application through the back wall")
	first_object.position = objects.bounded_position(first_object, Vector3(100, 100, -100))
	for corner_x in [-0.7, 0.7]:
		for corner_y in [-0.51, 0.51]:
			for corner_z in [-0.04, 0.04]:
				var corner := first_object.to_global(Vector3(corner_x, corner_y, corner_z))
				check(corner.x <= 3.861 and corner.y <= 3.151 and corner.z >= -4.361, "Panel extends outside room")
	first_object.position = saved
	check(not objects.placement_clear(first_object, second_object.position), "Placement overlaps another panel")
	check(not objects.placement_clear(first_object, Vector3(0, -0.08, 0)), "Placement intersects floor")
	second_object.position = first_object.position + objects.camera.global_position.direction_to(first_object.position) * 0.8
	check(objects.pick(point(first)) == first, "Nearest application does not win overlapping picks")
	first_object.rotation.y += PI
	check(objects.pick(point(first)) == 0, "Picked through an opaque panel back")
	first_object.rotation.y -= PI
	second_object.position = second_position
	# Rotation at fixed centers must respect solid walls and other panels.
	# The center of the front wall is now an open doorway; test beside it.
	var saved_transform: Transform3D = first_object.global_transform
	var saved_other: Transform3D = second_object.global_transform
	objects.pressed = first
	first_object.rotation = Vector3.ZERO
	first_object.position = Vector3(2, 1.7, 4.25)
	check(objects.placement_clear(first_object, first_object.position), "Wall rotation fixture is initially blocked")
	objects.rotate_drag(PI / 4)
	check(first_object.rotation.is_zero_approx() and first_object.position == Vector3(2, 1.7, 4.25), "Rotation crossed room bounds or moved center")
	first_object.position = Vector3(0, 1.7, 2)
	second_object.rotation = Vector3.ZERO
	second_object.position = Vector3(0, 1.7, 2.3)
	check(objects.placement_clear(first_object, first_object.position), "Panel rotation fixture is initially blocked")
	objects.rotate_drag(PI / 4)
	check(first_object.rotation.is_zero_approx() and first_object.position == Vector3(0, 1.7, 2), "Rotation overlaps another panel or moves center")
	objects.pressed = 0
	first_object.global_transform = saved_transform
	second_object.global_transform = saved_other
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
