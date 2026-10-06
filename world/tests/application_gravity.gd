## Real client panels, swept contacts against actual study furniture.
extends "res://tests/application_objects.gd"

func run() -> void:
	output_dir = ProjectSettings.globalize_path("res://../.tools/application-gravity-test")
	DirAccess.make_dir_recursive_absolute(output_dir)
	study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	objects = study.application_objects
	var deadline := Time.get_ticks_msec() + 15000
	while objects.bindings.is_empty() and Time.get_ticks_msec() < deadline: await process_frame
	root.grab_focus()
	await settle()
	check(not objects.bindings.is_empty(), "No real client panel")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	var window: int = terminal.handle
	var panel = objects.bindings[window]
	var player = study.get_node("Player")
	player.set_physics_process(false)
	await activate(window)
	await type_text("while true; do date +%T; sleep 0.2; done\n")
	await return_to_room()
	var revision: int = panel.revision
	var chair = study.get_node("Chair")
	var cases := [
		["floor", Vector3(-2, 2.3, 2), 0.0, study.get_node("Floor"), 0.0],
		["desk", Vector3(0.6, 2.3, -1.81), 0.0, study.get_node("Desk"), 0.7875],
		["bookcase", Vector3(2.9, 2.7, -3.85), 0.0, study.get_node("Bookshelf"), 2.0634],
		# The panel's center misses the seat; its left edge must still land on it.
		["chair", chair.to_global(Vector3(-0.45, 2.3, 0.12)), 0.2, chair, 0.45]
	]
	for spec in cases:
		panel.held = true
		panel.gravity_active = false
		panel.fall_speed = 0
		panel.position = spec[1]
		panel.rotation = Vector3(0, spec[2], 0)
		objects.pressed = window
		objects.dragging = true
		objects.original_gravity = false
		objects.finish_drag(false)
		var start: Vector3 = panel.position
		for frame in range(100): await physics_frame
		var support: Object = panel.support.get_ref() if panel.support != null else null
		check(support != null and (support == spec[3] or spec[3].is_ancestor_of(support)), "Wrong support for %s: %s" % [spec[0], support.get_path() if support != null else "none"])
		var bottom: float = panel.position.y - panel.bounds.y / 2
		check(absf(bottom - spec[4]) < 0.035, "%s contact floats/penetrates: bottom=%s" % [spec[0], bottom])
		check(panel.position.y < start.y and panel.fall_speed == 0, "Panel does not settle on " + spec[0])
		check(absf(panel.position.x - start.x) < 0.01 and absf(panel.position.z - start.z) < 0.01 and is_equal_approx(panel.rotation.y, spec[2]), "Falling changes heading or horizontal placement")
		player.camera.global_position = (panel.position + Vector3(1.2, 0.6, 2.7)).clamp(Vector3(-3.5, 0.5, -4), Vector3(3.5, 2.9, 4))
		player.camera.look_at(panel.position - Vector3(0, 0.1, 0))
		await capture("landed-" + spec[0] + ".png")
	# Push the actual supporting chair out from under the application's edge.
	var chair_start: Vector3 = chair.position
	for frame in range(90):
		# The controller applies this bounded impulse on each walking contact.
		chair.push(Vector3(-2.4, 0, 0), Vector3.RIGHT, 1.0 / 60.0)
		await physics_frame
	check(chair.position.x < chair_start.x - 0.8, "Walking push failed to clear the supporting chair")
	for frame in range(60): await physics_frame
	check(panel.position.y - panel.bounds.y / 2 < 0.025, "Removed chair support leaves panel floating: panel=%s chair=%s support=%s" % [panel.position, chair.position, panel.support.get_ref().get_path() if panel.support != null and panel.support.get_ref() != null else "none"])
	check(panel.revision > revision, "Falling panel lost live client updates")
	# Re-grab through real mouse dispatch, lift it and hold without falling.
	player.camera.global_position = panel.position + Vector3(0, 0.6, 2.5)
	player.camera.look_at(panel.position)
	await settle()
	var resting: Transform3D = panel.global_transform
	var pointer := point(window)
	await mouse(pointer, true)
	await motion(pointer + Vector2(0, -110))
	check(objects.pressed == window and panel.held and panel.position.y > resting.origin.y + 0.1, "Cannot lift a resting panel")
	var held_position: Vector3 = panel.position
	for frame in range(30): await physics_frame
	check(panel.position.is_equal_approx(held_position), "Held panel keeps falling")
	await tap(KEY_ESCAPE)
	await mouse(pointer, false)
	check(panel.global_transform.is_equal_approx(resting) and not panel.held and panel.gravity_active, "Cancellation loses settled placement/gravity")
	await activate(window)
	await chord(KEY_CTRL, KEY_C)
	await type_text("exit\n")
	deadline = Time.get_ticks_msec() + 5000
	while objects.bindings.has(window) and Time.get_ticks_msec() < deadline: await process_frame
	check(not objects.bindings.has(window), "Physical panel survives real client close")
	await terminal.shutdown()
	print("APPLICATION_GRAVITY_OK supports=4 failures=", failures)
	quit(1 if failures else 0)
