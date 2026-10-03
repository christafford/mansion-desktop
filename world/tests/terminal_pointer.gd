## Real Weston selection/scroll driven by injected Godot mouse events.
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

func motion(position: Vector2) -> void:
	var event := InputEventMouseMotion.new()
	event.position = position
	event.relative = Vector2(23, 17)
	event.screen_relative = event.relative
	Input.parse_input_event(event)
	await process_frame

func button(position: Vector2, code: int, pressed: bool) -> void:
	var event := InputEventMouseButton.new()
	event.position = position
	event.button_index = code
	event.pressed = pressed
	Input.parse_input_event(event)
	await process_frame

func settle() -> void:
	for i in range(12): await process_frame
	await RenderingServer.frame_post_draw

func capture(name: String) -> void:
	await settle()
	check(root.get_texture().get_image().save_png(output_dir.path_join(name)) == OK, "Capture " + name)

func point(local: Vector2) -> Vector2:
	var rect: Rect2 = app.image_rect()
	return rect.position + local / terminal.logical_size * rect.size

func difference(a: Image, b: Image, region: Rect2i) -> int:
	if a.get_size() != b.get_size(): return -1
	var count := 0
	for y in range(region.position.y, region.end.y):
		for x in range(region.position.x, region.end.x):
			if a.get_pixel(x,y) != b.get_pixel(x,y): count += 1
	return count

func run() -> void:
	if DisplayServer.get_name() == "headless":
		push_error("terminal_pointer requires a graphical renderer")
		quit(1)
		return
	output_dir = ProjectSettings.globalize_path("res://../.tools/terminal-pointer-test").simplify_path()
	check(DirAccess.make_dir_recursive_absolute(output_dir) == OK, "Test output directory")
	study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	var deadline := Time.get_ticks_msec() + 12000
	while terminal.updates < 1 and Time.get_ticks_msec() < deadline: await process_frame
	check(terminal.updates >= 1, "No terminal output")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	root.grab_focus()
	await settle()
	check(root.has_focus() and app.host_focused, "Host window must be focused for graphical input test")
	await button(Vector2(400, 300), MOUSE_BUTTON_LEFT, true)
	await button(Vector2(400, 300), MOUSE_BUTTON_LEFT, false)
	check(terminal.session.pointer_focus_handle() == 0, "World click reached client")
	await tap(KEY_ENTER)
	await button(Vector2(4, 4), MOUSE_BUTTON_LEFT, true)
	await button(Vector2(4, 4), MOUSE_BUTTON_LEFT, false)
	check(terminal.session.pointer_focus_handle() == 0 and not terminal.session.pointer_grabbed(), "Margin click reached client")
	check(app.active, "Enter did not activate application mode")
	await chord(KEY_CTRL, KEY_L)
	await type_text("printf 'SELECT THIS TEXT WITH THE MOUSE\\n'\n")
	await capture("before-selection.png")
	var before: Image = terminal.last_image.duplicate()
	check(before.save_png(output_dir.path_join("source-before.png")) == OK, "Save source frame")
	# Weston 15 monospace 16: second row, including its 32px surface shadow
	# and client decorations. Coordinates verified against source-before.png.
	var start := point(Vector2(43, 98))
	var end := point(Vector2(384, 98))
	check(app.active and app.host_focused, "Host focus/activation lost before selection")
	await motion(start)
	await button(start, MOUSE_BUTTON_LEFT, true)
	check(terminal.session.pointer_grabbed(), "Press did not establish grab")
	for i in range(1, 9): await motion(start.lerp(end, i / 8.0))
	await button(end, MOUSE_BUTTON_LEFT, false)
	await capture("selection.png")
	check(not terminal.session.pointer_grabbed(), "Release left grab held")
	var selected: Image = terminal.last_image.duplicate()
	var selection_pixels := difference(before, selected, Rect2i(40, 84, 370, 24))
	check(selection_pixels > 1000, "Drag did not highlight real terminal text")
	await motion(point(Vector2(450, 100)))
	await settle()
	check(difference(selected, terminal.last_image, Rect2i(40, 84, 370, 24)) == 0, "Selection kept dragging after release")
	# Another drag leaves the image; its release must reach the grabbed client.
	await create_timer(0.55).timeout # Avoid Weston's multi-click selection interval.
	await button(start, MOUSE_BUTTON_LEFT, true)
	await motion(Vector2(4, 4))
	check(terminal.session.pointer_grabbed(), "Leaving image dropped drag")
	await button(Vector2(4, 4), MOUSE_BUTTON_LEFT, false)
	check(not terminal.session.pointer_grabbed() and terminal.session.pointer_focus_handle() == 0, "Outside release retained grab/focus")
	await type_text("seq 1 100\n")
	await capture("scroll-bottom.png")
	var bottom: Image = terminal.last_image.duplicate()
	var center: Vector2 = app.image_rect().get_center()
	for i in range(3):
		await button(center, MOUSE_BUTTON_WHEEL_UP, true)
		await button(center, MOUSE_BUTTON_WHEEL_UP, false)
	await capture("scroll-up.png")
	var scroll_pixels := difference(bottom, terminal.last_image, Rect2i(40, 64, 70, 500))
	check(scroll_pixels > 1000, "Wheel did not change real scrollback")
	for i in range(3):
		await button(center, MOUSE_BUTTON_WHEEL_DOWN, true)
		await button(center, MOUSE_BUTTON_WHEEL_DOWN, false)
	await settle()
	check(difference(bottom, terminal.last_image, Rect2i(40, 64, 70, 500)) == 0, "Wheel down did not restore scrollback")
	await button(start, MOUSE_BUTTON_LEFT, true)
	check(terminal.session.pointer_grabbed(), "No held drag before mode exit")
	await key(KEY_CTRL, true)
	await key(KEY_ALT, true)
	await tap(KEY_ESCAPE)
	await key(KEY_ALT, false)
	await key(KEY_CTRL, false)
	check(not app.active and not terminal.session.pointer_grabbed() and terminal.session.pointer_focus_handle() == 0, "Mode exit retained pointer state")
	await button(start, MOUSE_BUTTON_LEFT, false)
	var player := study.get_node("Player")
	var camera_rotation: Vector3 = player.rotation
	var pitch: Vector3 = player.camera.rotation
	await motion(Vector2(900, 600))
	check(player.rotation == camera_rotation and player.camera.rotation == pitch and Input.mouse_mode == Input.MOUSE_MODE_VISIBLE, "World camera moved with released pointer")
	var position: Vector3 = player.position
	await key(KEY_W, true)
	for i in range(15): await process_frame
	await key(KEY_W, false)
	check(player.position.distance_to(position) > 0.05, "World movement did not resume")
	for i in range(10): await process_frame
	position = player.position
	for i in range(10): await process_frame
	check(player.position.distance_to(position) < 0.001, "World movement stuck")
	await capture("world-after-drag.png")
	await tap(KEY_ENTER)
	await button(start, MOUSE_BUTTON_LEFT, true)
	check(app.active and terminal.session.pointer_grabbed(), "No held drag before focus loss")
	app._notification(NOTIFICATION_APPLICATION_FOCUS_OUT)
	check(not app.active and not terminal.session.pointer_grabbed() and terminal.session.pointer_focus_handle() == 0, "Focus loss retained grab")
	await button(start, MOUSE_BUTTON_LEFT, false)
	app._notification(NOTIFICATION_APPLICATION_FOCUS_IN)
	await tap(KEY_ENTER)
	await button(start, MOUSE_BUTTON_LEFT, true)
	check(app.active and terminal.session.pointer_grabbed(), "No held drag before disconnect")
	check(OS.kill(terminal.child_pid) == OK, "Kill owned terminal")
	terminal.child_pid = -1
	deadline = Time.get_ticks_msec() + 4000
	while app.active and Time.get_ticks_msec() < deadline: await process_frame
	check(not app.active and not terminal.session.pointer_grabbed() and terminal.session.pointer_focus_handle() == 0, "Disconnect retained grab")
	await button(start, MOUSE_BUTTON_LEFT, false)
	await terminal.shutdown()
	print("TERMINAL_POINTER_OK selection_pixels=", selection_pixels, " scroll_pixels=", scroll_pixels, " failures=", failures)
	quit(1 if failures else 0)
