## Agent-run GPU integration test. Uses a real Weston terminal, not a fake image.
extends SceneTree
var failures := 0
func check(ok: bool, message: String) -> void:
	if not ok:
		push_error(message)
		failures += 1
func _initialize() -> void:
	call_deferred("run")
func run() -> void:
	if DisplayServer.get_name() == "headless":
		push_error("This test requires a graphical renderer")
		quit(1)
		return
	var original_env := OS.get_environment("WAYLAND_DISPLAY")
	var study: Node3D = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	var terminal = study.terminal_screen
	var deadline := Time.get_ticks_msec() + 12000
	while terminal.updates < 2 and Time.get_ticks_msec() < deadline:
		await process_frame
	check(terminal.updates >= 2, "Real terminal did not produce live frames")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	var pid: int = terminal.child_pid
	var socket_dir: String = terminal.session.socket_path().get_base_dir()
	var before: PackedByteArray = terminal.last_image.get_data()
	var previous_revision: int = terminal.revision
	var player = study.get_node("Player")
	player.set_physics_process(false)
	var camera: Camera3D = player.get_node("Camera3D")
	var destination := ProjectSettings.globalize_path("res://../docs/evidence/live-terminal").simplify_path()
	DirAccess.make_dir_recursive_absolute(destination)
	camera.global_position = Vector3(2.4, 1.65, 3.1)
	camera.look_at(Vector3(0, 1, -2))
	for i in range(8): await process_frame
	await RenderingServer.frame_post_draw
	check(root.get_texture().get_image().save_png(destination.path_join("room.png")) == OK, "Room capture")
	camera.global_position = Vector3(0, 1.26, -1.38)
	camera.look_at(Vector3(0, 1.2, -2.2))
	for i in range(8): await process_frame
	await RenderingServer.frame_post_draw
	check(root.get_texture().get_image().save_png(destination.path_join("monitor-before.png")) == OK, "Monitor capture")
	deadline = Time.get_ticks_msec() + 4500
	while (terminal.revision <= previous_revision or terminal.last_image.get_data() == before) and Time.get_ticks_msec() < deadline:
		await process_frame
	check(terminal.revision > previous_revision and terminal.last_image.get_data() != before, "Terminal pixels did not change")
	# Wait for a distinct clock/counter value in the saved close-up.
	await create_timer(2.1).timeout
	await RenderingServer.frame_post_draw
	check(root.get_texture().get_image().save_png(destination.path_join("monitor-after.png")) == OK, "Updated monitor capture")
	check(terminal.last_image.save_png(destination.path_join("client-frame.png")) == OK, "Source terminal frame capture")
	var mesh_size: Vector2 = terminal.screen.mesh.size
	check(absf(mesh_size.aspect() - terminal.last_image.get_size().aspect()) < 0.001, "Screen distorts client aspect ratio")
	check(terminal.session.snapshot(terminal.handle, terminal.revision).is_empty(), "Unchanged revision should not copy pixels")
	# Kill only the child this test launched; the live screen must clear on removal.
	check(OS.kill(pid) == OK, "Terminate owned test terminal")
	terminal.child_pid = -1 # OS.kill collected the owned process handle.
	deadline = Time.get_ticks_msec() + 3000
	while terminal.screen.visible and Time.get_ticks_msec() < deadline:
		await process_frame
	check(not terminal.screen.visible and terminal.texture == null, "Closed terminal left stale live content")
	await RenderingServer.frame_post_draw
	check(root.get_texture().get_image().save_png(destination.path_join("closed.png")) == OK, "Closed capture")
	await terminal.shutdown()
	var process_output: Array = []
	check(OS.execute("/usr/bin/kill", PackedStringArray(["-0", str(pid)]), process_output, true) != 0, "Owned terminal left running")
	check(not DirAccess.dir_exists_absolute(socket_dir), "Private socket directory leaked")
	var first_updates: int = terminal.updates
	# Start another real terminal and exercise orderly scene shutdown, without kill.
	study.queue_free()
	await process_frame
	study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	deadline = Time.get_ticks_msec() + 12000
	while terminal.updates < 1 and Time.get_ticks_msec() < deadline:
		await process_frame
	check(terminal.updates >= 1, "Second terminal failed to start")
	pid = terminal.child_pid
	socket_dir = terminal.session.socket_path().get_base_dir()
	await terminal.shutdown()
	check(pid > 0 and OS.execute("/usr/bin/kill", PackedStringArray(["-0", str(pid)]), process_output, true) != 0, "Shutdown left owned terminal running")
	check(not DirAccess.dir_exists_absolute(socket_dir), "Shutdown leaked private socket directory")
	check(OS.get_environment("WAYLAND_DISPLAY") == original_env, "Host display environment changed")
	print("LIVE_TERMINAL_OK updates=", first_updates, " sessions=2 failures=", failures)
	quit(1 if failures else 0)
