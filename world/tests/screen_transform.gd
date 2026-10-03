## Independent expected permutations for inverse Wayland buffer transforms.
extends SceneTree
const Screen = preload("res://scripts/terminal_screen.gd")
func _initialize() -> void:
	var bytes := PackedByteArray()
	for n in range(1, 7): bytes.append_array([n * 10, 0, 0, 255])
	var expected := [[1,2,3,4,5,6], [5,3,1,6,4,2], [6,5,4,3,2,1], [2,4,6,1,3,5],
		[2,1,4,3,6,5], [1,3,5,2,4,6], [5,6,3,4,1,2], [6,4,2,5,3,1]]
	var failures := 0
	for transform in range(8):
		var frame := {"width": 2, "height": 3, "transform": transform, "pixels": bytes}
		var image := Screen.image_from_frame(frame)
		var output := image.get_data()
		for i in range(6):
			if output[i * 4] != expected[transform][i] * 10:
				push_error("Wrong transform %d pixel %d" % [transform, i])
				failures += 1
		var size := Vector2i(3, 2) if transform % 2 else Vector2i(2, 3)
		if image.get_size() != size:
			failures += 1
	print("SCREEN_TRANSFORM_OK cases=8 failures=", failures)
	quit(1 if failures else 0)
