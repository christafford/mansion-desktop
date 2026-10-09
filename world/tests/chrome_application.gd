## Agent-injected input to installed Chrome; local HTML is test content only.
extends "res://tests/application_objects.gd"
var fixture_pid := -1

func page_state() -> Dictionary:
	var path := output_dir.path_join("page.json")
	if not FileAccess.file_exists(path): return {}
	var value = JSON.parse_string(FileAccess.get_file_as_string(path))
	return value if value is Dictionary else {}

func wait_page(field: String, value: Variant) -> bool:
	var deadline := Time.get_ticks_msec() + 8000
	while Time.get_ticks_msec() < deadline:
		if page_state().get(field) == value: return true
		await process_frame
	check(false, "Browser page did not report %s=%s: %s" % [field,value,page_state()])
	return false

func browser_launch(launcher: Node, identifier: String) -> int:
	launcher.open()
	launcher.launch(identifier)
	var deadline := Time.get_ticks_msec() + 15000
	while not launcher.pending.is_empty() and Time.get_ticks_msec() < deadline: await process_frame
	await settle()
	check(app.active, "Browser launch failed: " + launcher.notice.text)
	return terminal.handle

func finish() -> void:
	await terminal.shutdown()
	if fixture_pid > 0 and OS.is_process_running(fixture_pid): OS.kill(fixture_pid)
	fixture_pid = -1
	print("CHROME_APPLICATION_OK failures=", failures)
	quit(1 if failures else 0)

func run() -> void:
	if DisplayServer.get_name() == "headless":
		push_error("Chrome acceptance requires a graphical renderer")
		quit(1)
		return
	output_dir = ProjectSettings.globalize_path("res://../.tools/chrome-application-test").simplify_path()
	DirAccess.make_dir_recursive_absolute(output_dir)
	for name in ["port.txt", "page.json"]:
		if FileAccess.file_exists(output_dir.path_join(name)): DirAccess.remove_absolute(output_dir.path_join(name))
	OS.set_environment("MANSION_BROWSER_PROFILE_DIR", output_dir.path_join("profile"))
	fixture_pid = OS.create_process("/usr/bin/python3", PackedStringArray([ProjectSettings.globalize_path("res://../tests/browser_fixture.py"), output_dir]))
	study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	objects = study.application_objects
	var launcher = study.app_launcher
	var deadline := Time.get_ticks_msec() + 15000
	while (launcher.entries.is_empty() or terminal.updates < 1) and Time.get_ticks_msec() < deadline: await process_frame
	root.grab_focus()
	await settle()
	check(not launcher.entry("google-chrome.desktop").is_empty(), "Installed Google Chrome desktop entry required")
	if failures:
		await finish()
		return
	var first := await browser_launch(launcher, "google-chrome.desktop")
	if failures:
		await finish()
		return
	var port := (await wait_file("port.txt")).strip_edges()
	await chord(KEY_CTRL, KEY_L)
	await type_text("http://127.0.0.1:" + port + "/")
	await create_timer(0.3).timeout
	check(terminal.session.surface_count() > terminal.session.toplevel_count() + 1, "Address-bar child surface missing")
	await capture("address-bar.png")
	await tap(KEY_ENTER)
	await wait_page("value", "")
	await type_text("Mansion browser 42!")
	await wait_page("value", "Mansion browser 42!")
	check(page_state().get("trusted", false), "Text did not come from browser input events")
	await settle()
	# Locate the page's unique flat button color in actual client pixels, then
	# click through the same Godot pointer mapper as the owner uses.
	var pixels: Image = terminal.last_image
	var target := Vector2(-1,-1)
	for y in range(0, pixels.get_height(), 4):
		for x in range(0, pixels.get_width(), 4):
			var color := pixels.get_pixel(x,y)
			if absi(color.r8-20)<2 and absi(color.g8-120)<2 and absi(color.b8-200)<2:
				target = Vector2(x+8,y+8)
				break
		if target.x >= 0: break
	check(target.x >= 0, "Local page button not rendered")
	if target.x >= 0:
		var position: Vector2 = app.image_rect().position + target / Vector2(pixels.get_size()) * app.image_rect().size
		await motion(position)
		await mouse(position,true)
		await mouse(position,false)
		await wait_page("clicks", 1)
		check(page_state().get("trusted", false), "Click did not come from browser pointer input")
		await capture("browser-input.png")
		await mouse(position,true,false,MOUSE_BUTTON_WHEEL_DOWN,8)
		deadline = Time.get_ticks_msec() + 4000
		while page_state().get("scroll",0) < 100 and Time.get_ticks_msec() < deadline: await process_frame
		check(page_state().get("scroll",0) >= 100, "Browser wheel did not scroll")
	var old_width: int = page_state().get("width",0)
	root.size = Vector2i(1000,700)
	deadline = Time.get_ticks_msec() + 6000
	while page_state().get("width",0) == old_width and Time.get_ticks_msec() < deadline: await process_frame
	check(page_state().get("width",0) > 0 and page_state().get("width",0) < old_width, "Browser did not reflow after host resize")
	var resized: Dictionary = terminal.session.window_state(first)
	deadline = Time.get_ticks_msec() + 5000
	while (app.resize_pending or resized.committed_serial != resized.sent_serial) and Time.get_ticks_msec() < deadline:
		await process_frame
		resized = terminal.session.window_state(first)
	check(resized.requested_width >= 850, "Old buffer geometry created excessive resize margins")
	check(resized.committed_serial == resized.sent_serial, "Browser did not acknowledge and commit resize")
	# Chrome limits a single wheel event to a viewport-sized scroll.
	for attempt in range(6):
		if page_state().get("scroll",0) == 0: break
		await mouse(app.image_rect().get_center(),true,false,MOUSE_BUTTON_WHEEL_UP,32)
		await create_timer(0.3).timeout
	await wait_page("scroll", 0)
	await capture("browser-resized.png")
	await return_to_room()
	check(not app.active and objects.bindings.has(first), "Return to room lost browser")
	var identity: String = objects.bindings[first].entity_id
	await capture("browser-in-room.png")
	var second := await browser_launch(launcher, "google-chrome.desktop")
	check(second != first and objects.bindings.has(first) and objects.bindings.has(second), "Second launch did not open a separate browser window")
	check(objects.bindings[first].entity_id == identity, "Second browser changed first window's identity")
	await key(KEY_CTRL,true)
	await key(KEY_SHIFT,true)
	await tap(KEY_W)
	await key(KEY_SHIFT,false)
	await key(KEY_CTRL,false)
	deadline = Time.get_ticks_msec() + 5000
	while objects.bindings.has(second) and Time.get_ticks_msec() < deadline: await process_frame
	check(not objects.bindings.has(second) and objects.bindings.has(first), "Browser's own window close affected the wrong panel")
	await finish()

func _finalize() -> void:
	if fixture_pid > 0 and OS.is_process_running(fixture_pid): OS.kill(fixture_pid)
