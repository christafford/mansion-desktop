## Agent-rendered scene evidence and bounded fluorescent modulation.
extends "res://tests/study_smoke.gd"

func run() -> void:
	var study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	var player = study.get_node("Player")
	player.set_physics_process(false)
	var details = study.get_node("Details")
	details.set_process(false)
	var low := 1.0
	var high := 0.0
	for step in range(3600):
		for i in range(2):
			var strength: float = details.flicker(step / 60.0, i)
			low = minf(low, strength)
			high = maxf(high, strength)
	check(low > 0.94 and high <= 1.0 and high - low > 0.025, "Flicker must be subtle but nonconstant")
	for light in details.fixtures:
		check(light.shadow_enabled and (-light.global_basis.z).dot(Vector3.DOWN) > 0.999, "Ceiling light lacks downward shadows")
	var output := ProjectSettings.globalize_path("res://../.tools/study-details-test")
	DirAccess.make_dir_recursive_absolute(output)
	var positions := [Vector3(2.4, 1.65, 3.1), Vector3(0.5, 1.65, 0.2), Vector3(-2.8, 1.65, -3.4), Vector3(2.6, 1.55, 3.3), Vector3(0.8, 2.2, -0.4)]
	var targets := [Vector3(0, 1, -2), Vector3(0, 1.1, -2.2), Vector3(0.4, 1.1, -0.4), Vector3(0, 2.45, -1.2), Vector3(0, 0.2, -1.1)]
	for i in range(positions.size()):
		player.camera.global_position = positions[i]
		player.camera.look_at(targets[i])
		await capture(output.path_join("view-%d.png" % i))
	var shadows := root.get_texture().get_image()
	for light in details.fixtures: light.shadow_enabled = false
	await capture(output.path_join("shadows-disabled.png"))
	var flat := root.get_texture().get_image()
	var changed := 0
	for y in range(0, flat.get_height(), 2):
		for x in range(0, flat.get_width(), 2):
			if absf(shadows.get_pixel(x, y).get_luminance() - flat.get_pixel(x, y).get_luminance()) > 0.025: changed += 1
	check(changed > 300, "Downlight shadows did not change rendered room")
	await study.terminal_screen.shutdown()
	print("STUDY_DETAILS_OK flicker=", low, "..", high, " shadow_pixels=", changed, " failures=", failures)
	quit(1 if failures else 0)

func capture(path: String) -> void:
	for frame in range(12): await process_frame
	await RenderingServer.frame_post_draw
	check(root.get_texture().get_image().save_png(path) == OK, "Capture failed: " + path)
