## Real client reflow through configure/ack/commit, using shared input helpers.
extends "res://tests/terminal_pointer.gd"

func wait_resize(previous_serial: int) -> Dictionary:
	var deadline := Time.get_ticks_msec() + 5000
	var state: Dictionary = {}
	while Time.get_ticks_msec() < deadline:
		await process_frame
		state = terminal.session.window_state(terminal.handle)
		if not state.is_empty() and not app.resize_pending and state.sent_serial != previous_serial and state.committed_serial == state.sent_serial:
			await settle()
			return state
	check(false, "Client did not commit the resize configure")
	return state

func cell_size_file(name: String) -> Vector2i:
	var path := output_dir.path_join(name)
	var deadline := Time.get_ticks_msec() + 3000
	while not FileAccess.file_exists(path) and Time.get_ticks_msec() < deadline: await process_frame
	check(FileAccess.file_exists(path), "Shell did not report rows/columns")
	if not FileAccess.file_exists(path): return Vector2i.ZERO
	var numbers := FileAccess.get_file_as_string(path).strip_edges().split(" ", false)
	check(numbers.size() == 2, "Invalid stty output")
	return Vector2i(int(numbers[1]), int(numbers[0])) if numbers.size() == 2 else Vector2i.ZERO

func native_pixels() -> int:
	await settle()
	var captured := root.get_texture().get_image()
	var source: Image = terminal.last_image
	check(app.view.size == Vector2(source.get_size()), "Resized client should fit at native pixels: view=%s source=%s" % [app.view.size, source.get_size()])
	var count := 0
	var wrong := 0
	for y in range(0, source.get_height(), 16):
		for x in range(0, source.get_width(), 16):
			var expected := source.get_pixel(x, y)
			if expected.a8 != 255: continue
			var actual := captured.get_pixelv(Vector2i(app.view.position) + Vector2i(x,y))
			count += 1
			if absi(actual.r8 - expected.r8) > 2 or absi(actual.g8 - expected.g8) > 2 or absi(actual.b8 - expected.b8) > 2: wrong += 1
	check(count > 1000 and wrong == 0, "Resized client pixels distorted: %d/%d" % [wrong,count])
	return count

func run() -> void:
	if DisplayServer.get_name() == "headless":
		push_error("terminal_resize requires a graphical renderer")
		quit(1)
		return
	output_dir = ProjectSettings.globalize_path("res://../.tools/terminal-resize-test").simplify_path()
	check(DirAccess.make_dir_recursive_absolute(output_dir) == OK, "Output directory")
	for i in range(4):
		var path := output_dir.path_join("size%d" % i)
		if FileAccess.file_exists(path): check(DirAccess.remove_absolute(path) == OK, "Remove old size marker")
	study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	var deadline := Time.get_ticks_msec() + 12000
	while terminal.updates < 1 and Time.get_ticks_msec() < deadline: await process_frame
	check(terminal.updates > 0, "No terminal output")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	root.grab_focus()
	await settle()
	var initial: Dictionary = terminal.session.window_state(terminal.handle)
	await tap(KEY_ENTER)
	check(app.active, "Application activation")
	await wait_resize(initial.sent_serial)
	await type_text("cd '" + output_dir + "'\n")
	var previous := Vector2i.ZERO
	var sampled := 0
	var counts: Array[Vector2i] = []
	var viewports := [Vector2i(1000,700), Vector2i(1200,780), Vector2i(900,620), Vector2i(1280,800)]
	for i in range(viewports.size()):
		var state: Dictionary = terminal.session.window_state(terminal.handle)
		var old_image: Image = terminal.last_image
		root.size = viewports[i]
		state = await wait_resize(state.sent_serial)
		check(app.active and root.size == viewports[i], "Resize lost focus or host size")
		check(terminal.last_image != old_image, "Resize did not produce new client content")
		if i % 2:
			check(terminal.last_image.get_width() > old_image.get_width() and terminal.last_image.get_height() > old_image.get_height(), "Growing did not enlarge the committed client buffer")
		else:
			check(terminal.last_image.get_width() < old_image.get_width() and terminal.last_image.get_height() < old_image.get_height(), "Shrinking did not shrink the committed client buffer")
		check(terminal.logical_size.x >= state.width and terminal.logical_size.y >= state.height, "Window geometry exceeds committed surface")
		await chord(KEY_CTRL, KEY_L)
		await type_text("printf 'RESIZE_SELECTION_OK\\n'\n")
		await type_text("stty size | tee size%d\n" % i)
		var cells := await cell_size_file("size%d" % i)
		counts.append(cells)
		if i > 0:
			if i % 2: check(cells.x > previous.x and cells.y > previous.y, "Growing did not add columns and rows")
			else: check(cells.x < previous.x and cells.y < previous.y, "Shrinking did not remove columns and rows")
		previous = cells
		await settle()
		var before: Image = terminal.last_image.duplicate()
		# Window geometry excludes shadows; text row 2 is below decorations.
		var start := point(Vector2(state.x + 11, state.y + 66))
		var end := point(Vector2(state.x + 225, state.y + 66))
		await motion(start)
		await button(start, MOUSE_BUTTON_LEFT, true)
		await motion(end)
		await button(end, MOUSE_BUTTON_LEFT, false)
		await settle()
		check(difference(before, terminal.last_image, Rect2i(state.x + 8,state.y + 52,250,24)) > 1000, "Pointer selection missed after resize")
		sampled += await native_pixels()
		await capture("resize-%d.png" % i)
		var mesh: QuadMesh = terminal.screen.mesh
		check(is_equal_approx(mesh.size.x / mesh.size.y, terminal.logical_size.x / terminal.logical_size.y), "World preview aspect did not update")
	# Re-entry at the same viewport must not repeatedly configure rounded sizes.
	var final_state: Dictionary = terminal.session.window_state(terminal.handle)
	await key(KEY_CTRL,true)
	await key(KEY_ALT,true)
	await tap(KEY_ESCAPE)
	await key(KEY_ALT,false)
	await key(KEY_CTRL,false)
	check(not app.active and not terminal.session.pointer_grabbed(), "Return to room after resize")
	await capture("world-after-resize.png")
	await tap(KEY_ENTER)
	await create_timer(0.4).timeout
	check(terminal.session.window_state(terminal.handle).sent_serial == final_state.sent_serial, "Re-entry resent the same configure")
	await terminal.shutdown()
	print("TERMINAL_RESIZE_OK cells=", counts, " pixels=", sampled, " failures=", failures)
	quit(1 if failures else 0)
