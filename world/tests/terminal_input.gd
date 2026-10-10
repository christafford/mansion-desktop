## Injected Godot events drive a real shell through Wayland; not a human trial.
extends SceneTree
var failures := 0
var study: Node3D
var terminal: Node
var app: CanvasLayer
var held: Dictionary = {}
var output_dir := ""

func check(ok: bool, message: String) -> void:
	if not ok:
		push_error(message)
		failures += 1

func _initialize() -> void:
	call_deferred("run")

func key(code: int, pressed: bool, echo := false) -> void:
	if pressed: held[code] = true
	else: held.erase(code)
	var event := InputEventKey.new()
	event.physical_keycode = code
	event.keycode = code
	event.pressed = pressed
	event.echo = echo
	event.shift_pressed = held.has(KEY_SHIFT)
	event.ctrl_pressed = held.has(KEY_CTRL)
	event.alt_pressed = held.has(KEY_ALT)
	Input.parse_input_event(event)
	await process_frame

func tap(code: int) -> void:
	await key(code, true)
	await key(code, false)

func type_text(text: String) -> void:
	const SHIFTED := "~!@#$%^&*()_+{}|:\"<>?"
	const BASE := "`1234567890-=[]\\;',./"
	for c in text:
		var shift := c >= "A" and c <= "Z"
		var code := c.to_upper().unicode_at(0)
		var index := SHIFTED.find(c)
		if index >= 0:
			shift = true
			code = BASE.unicode_at(index)
		if c == "\n": code = KEY_ENTER
		if shift: await key(KEY_SHIFT, true)
		await tap(code)
		if shift: await key(KEY_SHIFT, false)

func chord(modifier: int, code: int) -> void:
	await key(modifier, true)
	await tap(code)
	await key(modifier, false)

func wait_file(name: String) -> String:
	var path := output_dir.path_join(name)
	var deadline := Time.get_ticks_msec() + 4000
	while not FileAccess.file_exists(path) and Time.get_ticks_msec() < deadline:
		await process_frame
	check(FileAccess.file_exists(path), "Shell did not create " + name)
	return FileAccess.get_file_as_string(path) if FileAccess.file_exists(path) else ""

func run() -> void:
	if DisplayServer.get_name() == "headless":
		push_error("terminal_input requires a graphical renderer")
		quit(1)
		return
	output_dir = ProjectSettings.globalize_path("res://../.tools/terminal-input-test").simplify_path()
	check(DirAccess.make_dir_recursive_absolute(output_dir) == OK, "Test output directory")
	for name in ["typed.txt", "repeat.txt", "interrupt.txt"]:
		if FileAccess.file_exists(output_dir.path_join(name)):
			check(DirAccess.remove_absolute(output_dir.path_join(name)) == OK, "Remove stale test marker")
	study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	var deadline := Time.get_ticks_msec() + 12000
	while terminal.updates < 1 and Time.get_ticks_msec() < deadline: await process_frame
	check(terminal.updates >= 1, "No real terminal output")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	# Match the pointer/launcher trials: wait for host activation before injection.
	root.grab_focus()
	await create_timer(0.25).timeout
	check(root.has_focus() and app.host_focused, "Keyboard test lacks host focus")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	await tap(KEY_ENTER)
	check(app.active, "Enter did not activate the terminal through Godot input")
	check(terminal.session.keyboard_focus_handle() == terminal.handle, "Wrong keyboard focus")
	var original_handle: int = terminal.handle
	var player := study.get_node("Player")
	var position: Vector3 = player.position
	var slow: bool = player.slow_walk
	await type_text("wasdM")
	await tap(KEY_HOME)
	await tap(KEY_TAB)
	await tap(KEY_ESCAPE)
	check(app.active and player.position.is_equal_approx(position) and player.slow_walk == slow, "Camera shortcuts stole application keys")
	await chord(KEY_CTRL, KEY_G) # Cancel readline's Escape prefix before editing.
	await tap(KEY_END)
	await chord(KEY_CTRL, KEY_U)
	await type_text("printf 'ELSEWHERE: AbC_42!\\n' | tee '" + output_dir.path_join("typed.txt") + "'X")
	await tap(KEY_BACKSPACE)
	await tap(KEY_ENTER)
	check(await wait_file("typed.txt") == "ELSEWHERE: AbC_42!\n", "Real shell typing/modifiers/backspace result")
	await type_text("printf '")
	await key(KEY_X, true)
	await key(KEY_X, true, true) # Host echo must not produce a second press.
	await create_timer(0.75).timeout
	await key(KEY_X, false)
	await type_text("' > '" + output_dir.path_join("repeat.txt") + "'\n")
	var repeated := await wait_file("repeat.txt")
	check(repeated.length() >= 3 and repeated.replace("x", "").is_empty(), "Client key repeat failed")
	await type_text("sleep 30\n")
	await create_timer(0.1).timeout
	await chord(KEY_CTRL, KEY_C)
	await type_text("printf 'INTERRUPT_OK\\n' | tee '" + output_dir.path_join("interrupt.txt") + "'\n")
	check(await wait_file("interrupt.txt") == "INTERRUPT_OK\n", "Ctrl+C did not interrupt the shell child")
	await key(KEY_CTRL, true)
	await key(KEY_ALT, true)
	await tap(KEY_ESCAPE)
	await key(KEY_ALT, false)
	await key(KEY_CTRL, false)
	check(not app.active and terminal.session.keyboard_focus_handle() == 0, "Reserved chord did not return to world")
	var world_start: Vector3 = player.position
	await key(KEY_W, true)
	for frame in range(15): await physics_frame
	await key(KEY_W, false)
	check(player.position.distance_to(world_start) > 0.2, "World movement did not resume")
	var world_stop: Vector3 = player.position
	for frame in range(10): await physics_frame
	check(player.position.distance_to(world_stop) < 0.06, "World movement stayed held")
	await tap(KEY_ENTER)
	check(app.active and terminal.handle == original_handle, "Same terminal failed to reactivate")
	await key(KEY_SHIFT, true)
	app._notification(Node.NOTIFICATION_APPLICATION_FOCUS_OUT)
	check(not app.active and terminal.session.keyboard_focus_handle() == 0, "Host focus loss left client focus")
	await key(KEY_SHIFT, false)
	app._notification(Node.NOTIFICATION_APPLICATION_FOCUS_IN)
	await tap(KEY_ENTER)
	check(app.active, "Terminal did not reactivate after host focus loss")
	await chord(KEY_CTRL, KEY_U)
	await type_text("printf 'RETURNED: lowercase and UPPERCASE\\n'\n")
	for frame in range(10): await process_frame
	check(app.active, "Typing unexpectedly left application mode")
	await RenderingServer.frame_post_draw
	var capture := root.get_texture().get_image()
	check(capture.save_png(output_dir.path_join("application.png")) == OK, "Application capture")
	check(terminal.last_image.save_png(output_dir.path_join("source.png")) == OK, "Terminal source capture")
	check(app.view.size == Vector2(terminal.last_image.get_size()), "Terminal should fit at native pixel size")
	var wrong_pixels := 0
	var checked_pixels := 0
	for y in range(0, terminal.last_image.get_height(), 8):
		for x in range(0, terminal.last_image.get_width(), 8):
			var expected: Color = terminal.last_image.get_pixel(x, y)
			if expected.a8 != 255: continue
			var actual := capture.get_pixelv(Vector2i(app.view.position) + Vector2i(x, y))
			checked_pixels += 1
			if absi(actual.r8 - expected.r8) > 2 or absi(actual.g8 - expected.g8) > 2 or absi(actual.b8 - expected.b8) > 2:
				wrong_pixels += 1
	check(checked_pixels > 1000 and wrong_pixels == 0, "Flat view altered client pixels: %d/%d" % [wrong_pixels, checked_pixels])
	await type_text("exit")
	await key(KEY_SHIFT, true) # Destruction while a modifier is held.
	await tap(KEY_ENTER)
	deadline = Time.get_ticks_msec() + 4000
	while app.active and Time.get_ticks_msec() < deadline: await process_frame
	check(not app.active and terminal.session.keyboard_focus_handle() == 0, "Client exit did not restore world mode")
	await key(KEY_SHIFT, false)
	check(player._held.is_empty(), "Client exit left world movement held")
	for frame in range(4): await process_frame
	await RenderingServer.frame_post_draw
	check(root.get_texture().get_image().save_png(output_dir.path_join("world-after-close.png")) == OK, "World after client exit capture")
	await terminal.shutdown()
	print("TERMINAL_INPUT_OK repeat_characters=", repeated.length(), " pixels=", checked_pixels, " failures=", failures)
	quit(1 if failures else 0)
