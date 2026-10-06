## Live-client flight, endpoints, interruption and lifecycle (agent-injected).
extends "res://tests/application_objects.gd"

func finish_flight() -> void:
	var deadline := Time.get_ticks_msec() + 2000
	while app.transition.running and Time.get_ticks_msec() < deadline: await process_frame
	check(not app.transition.running, "Application flight did not finish")
	await RenderingServer.frame_post_draw

func flight_capture(name: String) -> void:
	while app.transition.running and app.transition.elapsed < app.transition.DURATION * 0.4: await process_frame
	check(app.transition.running and app.transition.surface.visible, "No intermediate flight frame: " + name)
	await RenderingServer.frame_post_draw
	check(root.get_texture().get_image().save_png(output_dir.path_join(name)) == OK, "Flight capture failed")

func run() -> void:
	output_dir = ProjectSettings.globalize_path("res://../.tools/application-transition-test")
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
	check(not objects.bindings.is_empty() and app.host_focused, "Missing client/host focus")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	var window: int = terminal.handle
	var object: Node3D = objects.bindings[window]
	var transition = app.transition
	var player = study.get_node("Player")
	var player_pose: Transform3D = player.global_transform
	var original: Transform3D = object.global_transform
	check(app.enter_application(), "Could not enter application")
	check(app.active and transition.running and not app.panel.visible and object.presentation_hidden, "Entry did not start a world flight")
	var start: Transform3D = transition.surface.global_transform
	await mouse(app.view.get_global_rect().get_center(), true)
	check(not terminal.session.pointer_grabbed(), "Moving image accepted an accidental pointer grab")
	await mouse(app.view.get_global_rect().get_center(), false)
	await flight_capture("enter-middle.png")
	check(transition.surface.global_position.distance_to(start.origin) > 0.1, "Entry proxy did not approach camera")
	check(transition.surface.material_override == object.screen.material_override, "Flight is not using the live client material")
	await finish_flight()
	check(app.panel.visible and not transition.surface.visible and not object.presentation_hidden, "Entry failed to switch to native presentation")
	check(player.global_transform.is_equal_approx(player_pose) and object.global_transform.is_equal_approx(original), "Animation moved the player or physical panel")
	await settle()
	var target: Transform3D = transition.camera_pose()
	var top_left: Vector2 = objects.camera.unproject_position(target * Vector3(-0.5, 0.5, 0))
	var bottom_right: Vector2 = objects.camera.unproject_position(target * Vector3(0.5, -0.5, 0))
	check(top_left.distance_to(app.view.position) < 0.01 and bottom_right.distance_to(app.view.position + app.view.size) < 0.01, "3D endpoint does not match the native client rectangle")
	var actual := root.get_texture().get_image()
	var expected: Image = terminal.last_image
	var wrong := 0
	for y in range(0, expected.get_height(), 16):
		for x in range(0, expected.get_width(), 16):
			var pixel := expected.get_pixel(x, y)
			if pixel.a8 != 255: continue
			var shown := actual.get_pixelv(Vector2i(app.view.position) + Vector2i(x, y))
			if absf(shown.r - pixel.r) > 0.01 or absf(shown.g - pixel.g) > 0.01 or absf(shown.b - pixel.b) > 0.01: wrong += 1
	check(wrong == 0, "Native endpoint changed client pixels")
	await capture("application.png")
	await type_text("while true; do date +%T.%N; sleep 0.05; done\n")
	app.exit_application()
	check(not app.active and transition.running and not app.panel.visible, "Return did not start reverse flight")
	var revision: int = object.revision
	await flight_capture("return-middle.png")
	await finish_flight()
	await settle()
	check(object.revision > revision, "Background client stopped updating during return")
	check(object.global_transform.is_equal_approx(original) and not object.presentation_hidden, "Return lost room placement")
	await capture("room.png")
	# Reverse a partially completed flight without snapping its current pose.
	app.enter_application()
	for frame in range(4): await process_frame
	var interrupted: Transform3D = transition.surface.global_transform
	app.exit_application()
	check(transition.surface.global_transform.is_equal_approx(interrupted), "Rapid reversal snapped the surface")
	for frame in range(3): await process_frame
	interrupted = transition.surface.global_transform
	app.enter_application()
	check(transition.surface.global_transform.is_equal_approx(interrupted), "Rapid re-entry snapped the surface")
	await finish_flight()
	await chord(KEY_CTRL, KEY_C)
	# Resize during flight: endpoint must follow the actual new client rectangle.
	app.exit_application()
	await finish_flight()
	app.enter_application()
	var old_size := root.size
	root.size = Vector2i(1000, 700)
	await finish_flight()
	await create_timer(0.4).timeout
	check(app.active and app.panel.visible and app.view.size.x <= 968, "Resize interrupted application presentation")
	root.size = old_size
	await create_timer(0.4).timeout
	# The return target follows the latest world placement, not a saved entry pose.
	app.exit_application()
	object.position += Vector3(0.3, 0.3, 0)
	var moved: Transform3D = object.global_transform
	await finish_flight()
	var screen_size: Vector2 = object.screen.mesh.size
	var world_endpoint: Transform3D = object.screen.global_transform * Transform3D(Basis.from_scale(Vector3(screen_size.x, screen_size.y, 1)), Vector3.ZERO)
	check(transition.surface.global_transform.is_equal_approx(world_endpoint) and not object.presentation_hidden, "Return did not reach the moved panel")
	check(object.global_transform.is_equal_approx(moved), "Presentation overwrote latest placement")
	app.enter_application()
	for frame in range(3): await process_frame
	app._notification(MainLoop.NOTIFICATION_APPLICATION_FOCUS_OUT)
	check(not transition.running and not app.active and not object.presentation_hidden, "Focus loss left a hidden panel or animation")
	app._notification(MainLoop.NOTIFICATION_APPLICATION_FOCUS_IN)
	app.enter_application()
	await finish_flight()
	app.exit_application()
	await finish_flight()
	app.enter_application()
	await type_text("exit\n")
	deadline = Time.get_ticks_msec() + 4000
	while terminal.session.toplevel_handles().has(window) and Time.get_ticks_msec() < deadline: await process_frame
	await settle()
	check(not objects.bindings.has(window) and not app.active and not transition.running and not transition.surface.visible, "Client close left a flying surface or stale focus")
	await terminal.shutdown()
	print("APPLICATION_TRANSITION_OK failures=", failures)
	quit(1 if failures else 0)
