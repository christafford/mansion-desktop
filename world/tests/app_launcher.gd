## Actual desktop catalog and real Vim in Weston; injected events, not human input.
extends "res://tests/terminal_input.gd"

func settle() -> void:
	for frame in range(15): await process_frame
	await RenderingServer.frame_post_draw

func capture(name: String) -> void:
	await settle()
	check(root.get_texture().get_image().save_png(output_dir.path_join(name)) == OK, "Launcher capture " + name)

func click_at(position: Vector2) -> void:
	Input.warp_mouse(position)
	await settle()
	for pressed in [true, false]:
		var event := InputEventMouseButton.new()
		event.position = position
		event.global_position = position
		event.button_index = MOUSE_BUTTON_LEFT
		event.pressed = pressed
		Input.parse_input_event(event)
		await process_frame

func search_text(text: String) -> void:
	for character in text:
		for pressed in [true, false]:
			var event := InputEventKey.new()
			event.keycode = character.to_upper().unicode_at(0)
			event.physical_keycode = event.keycode
			event.unicode = character.unicode_at(0)
			event.pressed = pressed
			Input.parse_input_event(event)
			await process_frame

func run() -> void:
	if DisplayServer.get_name() == "headless":
		push_error("Launcher acceptance requires graphical rendering")
		quit(1)
		return
	output_dir = ProjectSettings.globalize_path("res://../.tools/launcher-test-captures").simplify_path()
	DirAccess.make_dir_recursive_absolute(output_dir)
	study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	var launcher = study.app_launcher
	var deadline := Time.get_ticks_msec() + 15000
	while (terminal.updates < 1 or launcher.entries.is_empty()) and Time.get_ticks_msec() < deadline:
		await process_frame
	check(terminal.updates > 0 and not launcher.entries.is_empty(), "No terminal or desktop application catalog")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	root.grab_focus()
	await settle()
	check(root.has_focus() and app.host_focused, "Launcher test lacks host focus")
	var original_handle: int = terminal.handle
	await tap(KEY_TAB)
	check(launcher.opened and launcher.wheel.visible, "World Tab did not open application wheel")
	check(launcher.wheel.get_child_count() == 9, "Wheel must have center plus eight slots")
	check(launcher.recent[0] == launcher.TERMINAL_ID, "Actual startup terminal not first in recents")
	var player := study.get_node("Player")
	var position: Vector3 = player.position
	var yaw: float = player.rotation.y
	await key(KEY_A, true)
	await settle()
	await key(KEY_A, false)
	check(player.position.is_equal_approx(position) and is_equal_approx(player.rotation.y, yaw), "Launcher leaked camera input")
	await capture("recent-applications.png")
	await click_at(launcher.wheel.global_position + launcher.wheel.size * 0.5)
	check(launcher.search_panel.visible and launcher.query.has_focus(), "Center click did not focus application search")
	await search_text("zzzz-no-such-app")
	check(launcher.results.item_count == 0, "Unmatched query not empty")
	await tap(KEY_ESCAPE)
	check(launcher.opened and launcher.wheel.visible, "Search Escape did not return to the wheel")
	await tap(KEY_ESCAPE)
	check(not launcher.opened and not player.application_mode and player._held.is_empty(), "Launcher cancel did not restore world controls")
	await tap(KEY_TAB)
	await click_at(launcher.wheel.global_position + launcher.wheel.size * 0.5)
	await search_text("Vim")
	var vim_index := -1
	for index in range(launcher.results.item_count):
		if launcher.results.get_item_metadata(index) == "vim.desktop": vim_index = index
	check(vim_index >= 0, "Application-name search did not find installed Vim: " + launcher.query.text)
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	launcher.results.select(vim_index)
	await capture("application-search.png")
	await tap(KEY_ENTER)
	deadline = Time.get_ticks_msec() + 14000
	while not launcher.pending.is_empty() and Time.get_ticks_msec() < deadline: await process_frame
	check(launcher.pending.is_empty() and app.active and terminal.handle != original_handle, "Selected editor failed to present: " + launcher.notice.text)
	check(launcher.recent[0] == "vim.desktop", "Successful launch not newest")
	if not failures:
		var revision: int = terminal.revision
		await type_text("iElsewhere launches real applications.\n")
		await settle()
		check(terminal.revision > revision, "Real editor did not repaint after typing")
		await capture("live-editor.png")
		await key(KEY_CTRL, true)
		await key(KEY_ALT, true)
		await tap(KEY_ESCAPE)
		await key(KEY_ALT, false)
		await key(KEY_CTRL, false)
		await tap(KEY_TAB)
		await capture("two-recent-applications.png")
		var terminal_slot: Control = launcher.wheel.get_child(2)
		await click_at(terminal_slot.get_global_rect().get_center())
		deadline = Time.get_ticks_msec() + 14000
		while not launcher.pending.is_empty() and Time.get_ticks_msec() < deadline: await process_frame
		var second_handle: int = terminal.handle
		check(app.active and second_handle != original_handle and terminal.session.toplevel_handles().has(original_handle), "Recent terminal did not create an independent window")
		check(launcher.live_windows.get(second_handle) == launcher.TERMINAL_ID and launcher.live_windows.get(original_handle) == launcher.TERMINAL_ID, "Both terminal instances must retain their labels")
		await settle()
		for name in ["instance-second.txt", "instance-first.txt"]:
			if FileAccess.file_exists(output_dir.path_join(name)): DirAccess.remove_absolute(output_dir.path_join(name))
		await type_text("ELSEWHERE_INSTANCE=SECOND; printf '%s\\n' \"$ELSEWHERE_INSTANCE\" > '" + output_dir.path_join("instance-second.txt") + "'\n")
		check(await wait_file("instance-second.txt") == "SECOND\n", "Second terminal did not accept independent input")
		launcher._activate(original_handle)
		await settle()
		await type_text("printf '%s\\n' \"${ELSEWHERE_INSTANCE-unset}\" > '" + output_dir.path_join("instance-first.txt") + "'\n")
		check(await wait_file("instance-first.txt") == "unset\n", "First terminal inherited the second terminal's shell state")
		launcher._activate(second_handle)
		await settle()
		await tap(KEY_TAB)
		check(not launcher.opened, "Terminal Tab was stolen by the launcher")
		await chord(KEY_CTRL, KEY_U)
		await type_text("exit\n")
		deadline = Time.get_ticks_msec() + 4000
		while terminal.session.toplevel_handles().has(second_handle) and Time.get_ticks_msec() < deadline: await process_frame
		check(not terminal.session.toplevel_handles().has(second_handle) and terminal.session.toplevel_handles().has(original_handle), "Closing one terminal affected its sibling")
		await settle()
		check(not launcher.live_windows.has(second_handle) and launcher.live_windows.has(original_handle), "Closed instance binding was not pruned")
		launcher.open()
		launcher.launch(launcher.TERMINAL_ID)
		deadline = Time.get_ticks_msec() + 14000
		while not launcher.pending.is_empty() and Time.get_ticks_msec() < deadline: await process_frame
		check(app.active and terminal.handle != original_handle, "Terminal failed to relaunch: active=%s host_focus=%s window_focus=%s handle=%s old=%s notice=%s" % [app.active, app.host_focused, root.has_focus(), terminal.handle, original_handle, launcher.notice.text])
		if failures:
			await capture("failed-relaunch.png")
			await terminal.shutdown()
			quit(1)
			return
		# A mapped Weston window can precede its shell prompt and activation resize.
		await settle()
		var marker := output_dir.path_join("relaunched.txt")
		if FileAccess.file_exists(marker): DirAccess.remove_absolute(marker)
		await type_text("printf 'LAUNCHER_OK\\n' > '" + marker + "'\n")
		check(await wait_file("relaunched.txt") == "LAUNCHER_OK\n", "Relaunched terminal did not accept real shell input")
		await capture("relaunched-terminal.png")
		check(launcher.recent[0] == launcher.TERMINAL_ID and launcher.recent.count(launcher.TERMINAL_ID) == 1, "Relaunch did not promote/deduplicate terminal history")
		app.exit_application()
		launcher.open()
	# Fixture history exercises capacity, persistence, ordering and all eight positions.
	var actual_history: Array = launcher.recent.duplicate()
	var expected: Array[String] = []
	for item in launcher.entries.slice(0, 10):
		launcher.remember(item.id)
		expected.push_front(item.id)
	expected.resize(8)
	check(launcher.recent == expected, "MRU history is not bounded/newest-first")
	launcher.remember(expected[3])
	check(launcher.recent.size() == 8 and launcher.recent[0] == expected[3], "Relaunch does not deduplicate/promote")
	var saved: Array = launcher.recent.duplicate()
	launcher.recent.clear()
	launcher._load_history()
	check(launcher.recent == saved, "History does not survive reload")
	await settle()
	var center: Vector2 = launcher.wheel.size * 0.5
	for index in range(8):
		var slot: Control = launcher.wheel.get_child(index + 1)
		var offset := slot.position + slot.size * 0.5 - center
		var target := Vector2.from_angle(-PI / 2 + index * TAU / 8)
		check(offset.normalized().distance_to(target) < 0.001, "Slot %d not clockwise from top" % index)
	launcher.heading.text = "Layout fixture · Eight slots (history injected)"
	await capture("fixture-eight-slots.png")
	launcher.recent.clear()
	actual_history.reverse()
	for identifier in actual_history: launcher.remember(identifier)
	launcher.close()
	await terminal.shutdown()
	check(terminal.launched_children.is_empty(), "Launcher left owned child processes")
	print("APP_LAUNCHER_OK entries=", launcher.entries.size(), " failures=", failures)
	quit(1 if failures else 0)
