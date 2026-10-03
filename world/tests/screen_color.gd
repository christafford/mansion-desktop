## GPU color test, using an explicitly synthetic flat test patch.
extends SceneTree
func _initialize():
	call_deferred("run")
func run():
	if DisplayServer.get_name() == "headless":
		push_error("This test requires a graphical renderer")
		quit(1)
		return
	var scene := Node3D.new()
	root.add_child(scene)
	var camera := Camera3D.new()
	scene.add_child(camera)
	camera.position.z = 2
	var env := WorldEnvironment.new()
	env.environment = Environment.new()
	env.environment.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	env.environment.tonemap_exposure = 1.0
	env.environment.tonemap_white = 1.0
	scene.add_child(env)
	var light := DirectionalLight3D.new()
	light.light_color = Color.RED
	light.light_energy = 8.0
	scene.add_child(light)
	var quad := MeshInstance3D.new()
	quad.mesh = QuadMesh.new()
	var mat := preload("res://scripts/screen_material.gd").create()
	quad.material_override = mat
	scene.add_child(quad)
	var failures := 0
	var colors: Array[Color] = []
	for value in [0, 16, 32, 64, 128, 192, 255]:
		colors.append(Color8(value, value, value))
	colors.append_array([Color.RED, Color.GREEN, Color.BLUE, Color8(37, 113, 219)])
	for expected in colors:
		var patch := Image.create(8, 8, false, Image.FORMAT_RGBA8)
		patch.fill(expected)
		mat.set_shader_parameter("client_pixels", ImageTexture.create_from_image(patch))
		for frame in range(3): await process_frame
		await RenderingServer.frame_post_draw
		var capture := root.get_texture().get_image()
		var color := capture.get_pixelv(capture.get_size() / 2)
		print("SCREEN_COLOR ", expected, " -> ", color.r8, " ", color.g8, " ", color.b8)
		if abs(color.r8 - expected.r8) > 2 or abs(color.g8 - expected.g8) > 2 or abs(color.b8 - expected.b8) > 2:
			push_error("Screen color altered by environment")
			failures += 1
	print("SCREEN_COLOR_OK failures=", failures)
	quit(1 if failures else 0)
