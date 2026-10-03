extends SceneTree
const Map = preload("res://scripts/pointer_map.gd")
const Screen = preload("res://scripts/terminal_screen.gd")
var failures := 0
var cases := 0
func check(ok: bool, message: String) -> void:
	cases += 1
	if not ok:
		push_error(message)
		failures += 1

func _initialize() -> void:
	# Actual texture permutation and logical size both come from a committed frame.
	var bytes := PackedByteArray()
	bytes.resize(80 * 40 * 4)
	for transform in range(8):
		for scale in [1, 2]:
			var frame := {"width": 80, "height": 40, "transform": transform, "pixels": bytes}
			var upright := Screen.image_from_frame(frame)
			var expected_size := Vector2(40, 80) if transform % 2 else Vector2(80, 40)
			var logical: Vector2 = expected_size / scale
			for view_size in [expected_size, Vector2(31, 27)]:
				var view := Rect2(Vector2(12, 34), view_size)
				var drawn := Map.drawn_rect(view, upright.get_size())
				check(drawn.size.x <= view_size.x and drawn.size.y <= view_size.y, "Fits native/shrunk view")
				check(Map.surface_position(drawn.position, drawn, logical).is_equal_approx(Vector2.ZERO), "Top left")
				check(Map.surface_position(drawn.get_center(), drawn, logical).is_equal_approx(logical * 0.5), "Center")
				for uv in [Vector2(0.999, 0), Vector2(0, 0.999), Vector2(0.999, 0.999)]:
					check(Map.surface_position(drawn.position + drawn.size * uv, drawn, logical).is_equal_approx(logical * uv), "Corner logical coordinates")
				check(not Map.surface_position(drawn.end, drawn, logical).is_finite(), "Exclusive bottom right")
				check(not Map.surface_position(drawn.position - Vector2.ONE, drawn, logical).is_finite(), "Margin rejected")
				check(Map.surface_position(drawn.position - drawn.size, drawn, logical, true).is_equal_approx(-logical), "Grab outside")
	var letterbox := Map.drawn_rect(Rect2(10, 20, 100, 100), Vector2(100, 50))
	check(letterbox == Rect2(10, 45, 100, 50), "Actual KEEP_ASPECT_CENTERED rectangle")
	check(not Map.surface_position(Vector2(50, 30), letterbox, Vector2(100, 50)).is_finite(), "Internal letterbox margin")
	check(not Map.surface_position(Vector2.INF, letterbox, Vector2.ONE, true).is_finite(), "Nonfinite input")
	check(not Map.surface_position(Vector2.ZERO, Rect2(), Vector2.ONE).is_finite(), "Empty image")
	for pair in [[MOUSE_BUTTON_LEFT,272], [MOUSE_BUTTON_RIGHT,273], [MOUSE_BUTTON_MIDDLE,274], [MOUSE_BUTTON_XBUTTON1,275], [MOUSE_BUTTON_XBUTTON2,276], [MOUSE_BUTTON_WHEEL_UP,0]]:
		check(Map.evdev(pair[0]) == pair[1], "Linux button mapping")
	check(Map.wheel(MOUSE_BUTTON_WHEEL_UP, 1) == Vector2(0,-10), "Wheel up")
	check(Map.wheel(MOUSE_BUTTON_WHEEL_DOWN, 0.5) == Vector2(0,5), "Fractional wheel down")
	check(Map.wheel(MOUSE_BUTTON_WHEEL_LEFT, 1) == Vector2(-10,0), "Wheel left")
	check(Map.wheel(MOUSE_BUTTON_WHEEL_RIGHT, 1) == Vector2(10,0), "Wheel right")
	print("POINTER_MAP_OK cases=", cases, " failures=", failures)
	quit(1 if failures else 0)
