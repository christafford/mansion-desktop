## Independent protocol keycodes for the supported physical US mapping.
extends SceneTree
const Map = preload("res://scripts/keyboard_map.gd")
func _initialize() -> void:
	var failures := 0
	var keys := [KEY_A, KEY_W, KEY_M, KEY_HOME, KEY_ESCAPE, KEY_TAB, KEY_ENTER,
		KEY_BACKSPACE, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_APOSTROPHE,
		KEY_BACKSLASH, KEY_1, KEY_SLASH, KEY_KP_ENTER, KEY_F12]
	var expected := [30, 17, 50, 102, 1, 15, 28, 14, 105, 106, 103, 108, 40, 43, 2, 53, 96, 88]
	for i in range(keys.size()):
		var event := InputEventKey.new()
		event.physical_keycode = keys[i]
		event.keycode = KEY_Z # Physical position takes precedence over layout symbol.
		if Map.evdev(event) != expected[i]: failures += 1
	for pair in [[KEY_SHIFT, 42, 54], [KEY_CTRL, 29, 97], [KEY_ALT, 56, 100], [KEY_META, 125, 126]]:
		var event := InputEventKey.new()
		event.keycode = pair[0] # Fallback for injected events lacking physical code.
		if Map.evdev(event) != pair[1]: failures += 1
		event.location = KEY_LOCATION_RIGHT
		if Map.evdev(event) != pair[2]: failures += 1
	if Map.evdev(InputEventKey.new()) != 0: failures += 1
	print("KEYBOARD_MAP_OK cases=27 failures=", failures)
	quit(1 if failures else 0)
