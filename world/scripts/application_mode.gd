## Owns shell input policy; compositor resources remain behind the session API.
extends CanvasLayer
const KeyboardMap = preload("res://scripts/keyboard_map.gd")
const PointerMap = preload("res://scripts/pointer_map.gd")
var terminal: Node
var player: CharacterBody3D
var active := false
var host_focused := true
var focused_handle := 0
var panel: Control
var view: TextureRect

func _ready() -> void:
	layer = 10
	panel = Control.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(panel)
	var background := ColorRect.new()
	background.color = Color(0.025, 0.03, 0.035, 1)
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	panel.add_child(background)
	var help := Label.new()
	help.text = "Terminal  •  US keyboard  •  Ctrl+Alt+Esc returns to the room\nDrag to select  •  Wheel to scroll"
	help.position = Vector2(22, 12)
	help.add_theme_font_size_override("font_size", 16)
	panel.add_child(help)
	view = TextureRect.new()
	view.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	view.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	view.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR
	view.mouse_filter = Control.MOUSE_FILTER_IGNORE
	panel.add_child(view)
	panel.hide()
	get_viewport().mouse_exited.connect(pointer_left_window)

func enter_application() -> bool:
	if not host_focused or terminal.session == null or not terminal.screen.visible:
		return false
	if not terminal.session.focus_keyboard(terminal.handle):
		push_error(terminal.session.last_error())
		return false
	focused_handle = terminal.handle
	active = true
	player.application_mode = true
	player.release_pointer()
	panel.show()
	update_view()
	return true

func exit_application() -> void:
	if terminal.session != null and terminal.session.is_running():
		terminal.session.pointer_reset()
		if not terminal.session.focus_keyboard(0):
			push_error(terminal.session.last_error())
	active = false
	focused_handle = 0
	player.application_mode = false
	player.release_pointer()
	panel.hide()
	view.texture = null

func update_view() -> void:
	view.texture = terminal.texture
	if view.texture == null:
		return
	var available := get_viewport().get_visible_rect().size - Vector2(32, 88)
	var native_size: Vector2 = view.texture.get_size()
	var scale := minf(1.0, minf(available.x / native_size.x, available.y / native_size.y))
	view.size = (native_size * maxf(scale, 0.01)).floor()
	view.position = (Vector2(16, 64) + (available - view.size) * 0.5).floor()

func _process(_delta: float) -> void:
	if not active:
		return
	if terminal.session == null or not terminal.session.is_running() or not terminal.screen.visible or terminal.session.keyboard_focus_handle() != focused_handle:
		exit_application()
		return
	update_view()

func _input(event: InputEvent) -> void:
	if not host_focused:
		return
	if event is InputEventKey:
		var key: int = event.physical_keycode if event.physical_keycode else event.keycode
		if not active:
			if event.pressed and not event.echo and key == KEY_ENTER:
				if enter_application(): get_viewport().set_input_as_handled()
			return
		get_viewport().set_input_as_handled()
		if event.pressed and key == KEY_ESCAPE and event.ctrl_pressed and event.alt_pressed:
			exit_application()
			return
		# Wayland clients repeat from repeat_info; host echo would double-repeat.
		if event.echo:
			return
		var code := KeyboardMap.evdev(event)
		if code and not terminal.session.keyboard_key(code, event.pressed):
			exit_application()
	elif active and event is InputEventMouse:
		get_viewport().set_input_as_handled()
		route_pointer(event)
	elif active and event is InputEventPanGesture:
		get_viewport().set_input_as_handled()
		if move_pointer(event.position) and image_rect().has_point(event.position):
			if not terminal.session.pointer_axis(event.delta.x * 10, event.delta.y * 10): exit_application()
	elif active and (event is InputEventJoypadButton or event is InputEventJoypadMotion):
		get_viewport().set_input_as_handled()

func image_rect() -> Rect2:
	if view.texture == null: return Rect2()
	return PointerMap.drawn_rect(view.get_global_rect(), view.texture.get_size())

func move_pointer(position: Vector2) -> bool:
	var grabbed: bool = terminal.session.pointer_grabbed()
	var local := PointerMap.surface_position(position, image_rect(), terminal.logical_size, grabbed)
	if not local.is_finite():
		if not grabbed: terminal.session.pointer_motion(0, 0, 0)
		return false
	if not terminal.session.pointer_motion(focused_handle, local.x, local.y):
		exit_application()
		return false
	return true

func route_pointer(event: InputEventMouse) -> void:
	if not move_pointer(event.position): return
	if event is InputEventMouseButton:
		var inside := image_rect().has_point(event.position)
		var code := PointerMap.evdev(event.button_index)
		# Outside motion/releases complete a drag, but margins never start clicks.
		if code and (inside or not event.pressed):
			if not terminal.session.pointer_button(code, event.pressed):
				exit_application()
				return
		elif event.pressed and inside:
			var axis := PointerMap.wheel(event.button_index, event.factor)
			if axis != Vector2.ZERO and not terminal.session.pointer_axis(axis.x, axis.y):
				exit_application()
				return
		if not inside and not terminal.session.pointer_grabbed():
			terminal.session.pointer_motion(0, 0, 0)

func pointer_left_window() -> void:
	if active and not terminal.session.pointer_grabbed():
		terminal.session.pointer_motion(0, 0, 0)

func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT:
		host_focused = false
		if active: exit_application()
	elif what == NOTIFICATION_APPLICATION_FOCUS_IN:
		host_focused = true
