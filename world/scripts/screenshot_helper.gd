extends Node3D

var camera: Camera3D
var capture_index: int = 0
var max_captures: int = 3

func _ready() -> void:
	camera = $Camera3D
	_render_view()

func _render_view() -> void:
	if capture_index >= max_captures:
		print("All captures done")
		get_tree().quit()
		return
	
	# Set camera based on capture index
	match capture_index:
		0:
			# Arrival view
			camera.position = Vector3(0, 1.6, 5)
			camera.rotation = Vector3(0, 0, 0)
		1:
			# Desk close-up
			camera.position = Vector3(0, 1.6, 1.0)
			camera.rotation = Vector3(0, 0, 0)
		2:
			# Opposite corner
			camera.position = Vector3(-5, 1.6, 5)
			camera.rotation = Vector3(0, -2.35619, 0)
	
	# Wait a frame for camera to update
	await get_tree().process_frame
	
	# Get viewport texture
	var viewport = get_viewport()
	var vt = viewport.get_texture()
	
	# Try to get texture data
	var image = vt.get_image()
	if image:
		image.flip_y()
		var filename = "capture_" + str(capture_index + 1) + ".png"
		var result = image.save_png(filename)
		if result == OK:
			print("Saved: ", filename)
		else:
			print("Failed: ", filename, " result: ", result)
	
	capture_index += 1
	_render_view()
