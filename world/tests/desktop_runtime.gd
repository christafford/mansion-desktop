## Real-client lifecycle without loading the study, its assets, or its bounds.
extends SceneTree

var failures := 0

func check(ok: bool, message: String) -> void:
	if not ok:
		push_error(message)
		failures += 1

func _initialize() -> void:
	call_deferred("run")

func run() -> void:
	var original_display := OS.get_environment("WAYLAND_DISPLAY")
	for orderly in [true, false]:
		# Presentation-only fixture: no replacement location or default scene.
		var fixture := Node3D.new()
		var player := preload("res://scripts/game_world.gd").new()
		var camera := Camera3D.new()
		camera.name = "Camera3D"
		player.add_child(camera)
		fixture.add_child(player)
		var screen := MeshInstance3D.new()
		screen.mesh = QuadMesh.new()
		screen.material_override = preload("res://scripts/screen_material.gd").create()
		fixture.add_child(screen)
		var status := Label3D.new()
		fixture.add_child(status)
		var desktop := preload("res://scripts/desktop_runtime.gd").new()
		fixture.add_child(desktop)
		root.add_child(fixture)
		player.set_physics_process(false)
		var bounds := AABB(Vector3(-8, 0, -8), Vector3(16, 8, 16))
		desktop.initialize(player, screen, status, func(center: Vector3, extents: Vector3) -> Vector3:
			return center.clamp(bounds.position + extents, bounds.end - extents))
		var terminal = desktop.terminal_screen
		var session = terminal.session
		var deadline := Time.get_ticks_msec() + 12000
		while terminal.updates == 0 and Time.get_ticks_msec() < deadline:
			await process_frame
		check(terminal.updates > 0, "World-neutral desktop did not map a real terminal")
		var pid: int = terminal.child_pid
		var socket_dir: String = session.socket_path().get_base_dir() if session != null else ""
		var objects = desktop.application_objects
		check(objects.bindings.has(terminal.handle), "Desktop did not create the live panel")
		if objects.bindings.has(terminal.handle):
			var object: Node3D = objects.bindings[terminal.handle]
			object.rotation.y = PI / 4
			var extents: Vector3 = objects.placement_extents(object)
			var point: Vector3 = objects.bounded_position(object, Vector3(100, -100, 100))
			check(point.is_equal_approx(Vector3(bounds.end.x - extents.x, extents.y, bounds.end.z - extents.z)), "Panel placement ignored the supplied world bounds")
			check(object.entity_id.length() == 32, "Panel lost its independent entity identity")
		if orderly:
			await desktop.shutdown()
			await desktop.shutdown() # Idempotent explicit shutdown.
		# Removing the service subtree must also clean up without scene cooperation.
		desktop.queue_free()
		await process_frame
		await process_frame
		check(session != null and not session.is_running(), "Desktop removal left the compositor running")
		var process_output: Array = []
		check(pid > 0 and OS.execute("/usr/bin/kill", PackedStringArray(["-0", str(pid)]), process_output, true) != 0, "Desktop removal left its owned terminal running")
		check(not socket_dir.is_empty() and not DirAccess.dir_exists_absolute(socket_dir), "Desktop removal leaked the private socket directory")
		check(OS.get_environment("WAYLAND_DISPLAY") == original_display, "Desktop changed the host display environment")
		fixture.queue_free()
		await process_frame
	print("DESKTOP_RUNTIME_OK sessions=2 failures=", failures)
	quit(1 if failures else 0)
