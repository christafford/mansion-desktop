## Real Wayland socket/client lifecycle driven by Godot frames; no pixel claim.
extends SceneTree
var failures := 0
var heartbeats := 0
var longest_pump_us := 0
var session
var child_pid := -1

func check(ok: bool, message: String) -> bool:
	if not ok:
		push_error(message)
		failures += 1
	return ok

func tick() -> void:
	await process_frame
	heartbeats += 1
	if session != null and session.is_running():
		var started := Time.get_ticks_usec()
		check(session.pump(), session.last_error())
		longest_pump_us = maxi(longest_pump_us, Time.get_ticks_usec() - started)

func wait_for_marker(path: String) -> bool:
	var deadline := Time.get_ticks_msec() + 5000
	while not FileAccess.file_exists(path) and Time.get_ticks_msec() < deadline:
		await tick()
	return check(FileAccess.file_exists(path), "Fixture did not reach " + path)

func _initialize() -> void:
	call_deferred("run")

func run() -> void:
	var args := OS.get_cmdline_user_args()
	if not check(args.size() == 2, "Expected fixture executable and runtime directory"):
		quit(1)
		return
	if not check(ClassDB.class_exists("MansionCompositorSession"), "Missing native compositor class"):
		quit(1)
		return
	session = ClassDB.instantiate("MansionCompositorSession")
	check(not session.pump(), "Stopped pump succeeded")
	check(not session.start("relative"), "Relative runtime directory accepted")
	for trial in range(16):
		if not check(session.start(args[1]), session.last_error()):
			break
		check(not session.start(args[1]), "Double start accepted")
		var socket: String = session.socket_path()
		var private_dir := socket.get_base_dir()
		var marker := args[1].path_join("trial-%d" % trial)
		var mode: String = ["destroy", "disconnect", "server-stop", "stale-ack", "unknown-ack", "unconfigured", "roleless", "bad-geometry"][trial % 8]
		child_pid = OS.create_process(args[0], PackedStringArray([socket, marker, mode]))
		if not check(child_pid > 0, "Could not spawn owned fixture"):
			break
		if not await wait_for_marker(marker + ".ready"):
			break
		check(session.client_count() == 1, "Fixture did not connect to private server")
		check(session.surface_count() == 16, "Fixture surface requests were not served")
		check(session.toplevel_count() == 16, "Toplevel registrations missing")
		if mode == "server-stop":
			check(session.stop(), session.last_error())
		var barrier := FileAccess.open(marker + ".go", FileAccess.WRITE)
		if not check(barrier != null, "Could not release fixture barrier"):
			break
		barrier.close()
		if mode == "destroy":
			if not await wait_for_marker(marker + ".partial"):
				break
			check(session.toplevel_count() == 8 and session.surface_count() == 8, "Non-first window removal failed")
			barrier = FileAccess.open(marker + ".resume", FileAccess.WRITE)
			if not check(barrier != null, "Could not release partial destruction barrier"):
				break
			barrier.close()
		if not await wait_for_marker(marker + ".ok"):
			break
		var deadline := Time.get_ticks_msec() + 5000
		while OS.is_process_running(child_pid) and Time.get_ticks_msec() < deadline:
			await tick()
		check(not OS.is_process_running(child_pid), "Owned fixture did not exit")
		if OS.is_process_running(child_pid):
			OS.kill(child_pid)
		child_pid = -1
		for frame in range(3):
			await tick()
		check(session.toplevel_count() == 0, "Client destruction leaked toplevels")
		check(session.surface_count() == 0, "Client destruction leaked surfaces")
		check(session.client_count() == 0, "Client destruction leaked connection")
		check(session.stop() and session.stop(), "Stop is not idempotent")
		check(not DirAccess.dir_exists_absolute(private_dir), "Socket directory was not removed")
		for suffix in [".ready", ".go", ".partial", ".resume", ".ok"]:
			DirAccess.remove_absolute(marker + suffix)
	if child_pid > 0 and OS.is_process_running(child_pid):
		OS.kill(child_pid)
	check(session.stop(), "Final cleanup failed")
	check(session.start(args[1]), "Start before implicit destructor failed")
	var last_dir: String = session.socket_path().get_base_dir()
	session = null
	check(not DirAccess.dir_exists_absolute(last_dir), "RefCounted destructor leaked socket directory")
	check(heartbeats >= 16, "Godot frames did not advance during client work")
	check(longest_pump_us < 500000, "Server pump stalled the Godot loop")
	print("GODOT_LIFECYCLE_OK trials=16 frames=", heartbeats, " max_pump_us=", longest_pump_us, " failures=", failures)
	quit(1 if failures else 0)
