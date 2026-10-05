## Live previews, world picking and placement; no compositor resource ownership.
extends Node3D

const ApplicationObject = preload("res://scripts/application_object.gd")
var terminal: Node
var app: CanvasLayer
var launcher: CanvasLayer
var player: CharacterBody3D
var camera: Camera3D
# Runtime bindings are separate from each object's independent entity_id.
var bindings: Dictionary = {}
var selected := 0
var pressed := 0
var dragging := false
var press_position := Vector2.ZERO
var original_transform := Transform3D.IDENTITY
var drag_offset := Vector3.ZERO
var drag_pointer := Vector2.ZERO
var drag_camera_transform := Transform3D.IDENTITY
var drag_depth := 2.0
var next_number := 1
var hint: Label

func _ready() -> void:
	camera = player.get_node("Camera3D")
	var ui := CanvasLayer.new()
	add_child(ui)
	hint = Label.new()
	hint.position = Vector2(22, 116)
	hint.add_theme_font_size_override("font_size", 16)
	hint.add_theme_color_override("font_shadow_color", Color.BLACK)
	hint.add_theme_constant_override("shadow_offset_y", 2)
	hint.mouse_filter = Control.MOUSE_FILTER_IGNORE
	ui.add_child(hint)

func _process(_delta: float) -> void:
	if app.active: selected = app.focused_handle
	hint.visible = not app.active and not launcher.opened
	hint.text = "Double-click to use · Drag to move · Esc cancels\nWhile dragging: WASD move · Right mouse look\n"
	hint.text += "Both buttons + wheel: up turns left / down turns right" if pressed != 0 and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED else "Wheel: up away / down closer · Both buttons + wheel: rotate"
	if pressed != 0 and (app.active or launcher.opened or not app.host_focused): finish_drag(true)
	if terminal.session == null or not terminal.session.is_running():
		clear_objects()
		return
	var windows: PackedInt64Array = terminal.session.toplevel_handles()
	for window in bindings.keys():
		if not windows.has(window): remove_object(window)
	for window in windows:
		var object: Node3D = bindings.get(window)
		var frame: Dictionary = terminal.session.snapshot(window, object.revision if object != null else 0)
		if frame.is_empty(): continue
		if object == null:
			if not frame.mapped: continue
			object = ApplicationObject.new()
			add_child(object)
			object.label.text = "Application %d" % next_number
			next_number += 1
			object.rotation.y = player.rotation.y
			object.position = initial_position(object)
			bindings[window] = object
		# The selected preview shares the already-uploaded monitor texture.
		object.show_frame(frame, terminal.texture if window == terminal.handle else null)
		if not object.visible and pressed == window: finish_drag(true)
	for identifier in launcher.live_windows:
		var window: int = launcher.live_windows[identifier]
		if bindings.has(window):
			bindings[window].label.text = str(launcher.entry(identifier).get("name", "Application")).left(38)
	for window in bindings:
		bindings[window].select(window == selected)

func _physics_process(_delta: float) -> void:
	if pressed == 0 or app.active or launcher.opened or not app.host_focused: return
	# Keep the grabbed point relative to the view even without a mouse-motion event.
	if not camera.global_transform.is_equal_approx(drag_camera_transform):
		dragging = true
		drag_camera_transform = camera.global_transform
	if dragging: move_drag(drag_pointer)

func placement_extents(object: Node3D) -> Vector3:
	var half := ApplicationObject.BOUNDS * 0.5
	return object.global_basis.x.abs() * half.x + object.global_basis.y.abs() * half.y + object.global_basis.z.abs() * half.z

func bounded_position(object: Node3D, point: Vector3) -> Vector3:
	var extents := placement_extents(object)
	return point.clamp(Vector3(-3.86, 0.08, -4.36) + extents, Vector3(3.86, 3.15, 4.36) - extents)

func placement_clear(object: Node3D, point: Vector3) -> bool:
	var query := PhysicsShapeQueryParameters3D.new()
	var shape := BoxShape3D.new()
	shape.size = ApplicationObject.BOUNDS + Vector3.ONE * 0.04
	query.shape = shape
	query.transform = Transform3D(object.global_basis, point)
	# Include the player: panels must not materialize inside the camera.
	if not get_world_3d().direct_space_state.intersect_shape(query, 1).is_empty(): return false
	var extents := placement_extents(object) + Vector3.ONE * 0.02
	var bounds := AABB(point - extents, extents * 2)
	for other in bindings.values():
		var other_extents := placement_extents(other)
		if other != object and bounds.intersects(AABB(other.global_position - other_extents, other_extents * 2)):
			return false
	return true

func initial_position(object: Node3D) -> Vector3:
	var forward := -player.global_basis.z
	var right := player.global_basis.x
	# Prefer nearby, visible space. Existing panels keep their placements.
	for depth in [2.4, 3.9, 5.4]:
		for side in [0.0, -1.55, 1.55, -3.1, 3.1]:
			var point := bounded_position(object, camera.global_position + forward * depth + right * side)
			if placement_clear(object, point) and visible_placement(object, point): return point
	# Crowded rooms may overlap previews, but every live window remains accessible
	# through the launcher and can be dragged to a new position.
	return bounded_position(object, camera.global_position + forward * 1.6)

func visible_placement(object: Node3D, point: Vector3) -> bool:
	var viewport := get_viewport().get_visible_rect().grow(-20)
	for x in [-0.7, 0.7]:
		for y in [-0.51, 0.51]:
			var corner := point + object.global_basis * Vector3(x, y, 0)
			if camera.is_position_behind(corner) or not viewport.has_point(camera.unproject_position(corner)): return false
	var origin := camera.global_position
	var direction := origin.direction_to(point)
	for other in bindings.values():
		if other.occlusion_distance(origin, direction) < origin.distance_to(point): return false
	return true

func pick(position: Vector2) -> int:
	var origin := camera.project_ray_origin(position)
	var direction := camera.project_ray_normal(position)
	var query := PhysicsRayQueryParameters3D.create(origin, origin + direction * 20)
	query.exclude = [player.get_rid()]
	var obstacle := get_world_3d().direct_space_state.intersect_ray(query)
	var nearest: float = origin.distance_to(obstacle.position) if not obstacle.is_empty() else 20.0
	var result := 0
	for window in bindings:
		var distance: float = bindings[window].occlusion_distance(origin, direction)
		if distance < nearest:
			nearest = distance
			result = window if bindings[window].ray_distance(origin, direction) < INF else 0
	return result

func _input(event: InputEvent) -> void:
	# Capture transitions can emit mouse_exited while left remains held. End on
	# release/cancellation; GUI controls must not swallow the release.
	if pressed != 0 and event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT and not event.pressed:
		finish_drag(false)
		get_viewport().set_input_as_handled()

func _unhandled_input(event: InputEvent) -> void:
	if app.active or launcher.opened or not app.host_focused: return
	if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED and pressed == 0: return
	if event is InputEventKey and pressed != 0:
		var key: int = event.physical_keycode if event.physical_keycode else event.keycode
		if event.pressed and key == KEY_ESCAPE:
			finish_drag(true)
			get_viewport().set_input_as_handled()
	elif event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_LEFT:
			if event.pressed:
				var window := pick(event.position)
				if window == 0: return
				selected = window
				terminal.select_window(window)
				if event.double_click:
					finish_drag(false)
					app.enter_application()
				else:
					pressed = window
					press_position = event.position
					original_transform = bindings[window].global_transform
					drag_depth = -camera.to_local(original_transform.origin).z
					drag_pointer = event.position
					drag_camera_transform = camera.global_transform
					drag_offset = camera.global_basis.inverse() * (original_transform.origin - camera.project_position(drag_pointer, drag_depth))
			elif pressed != 0:
				finish_drag(false)
			else: return
			get_viewport().set_input_as_handled()
		elif pressed != 0 and event.button_index in [MOUSE_BUTTON_WHEEL_UP, MOUSE_BUTTON_WHEEL_DOWN]:
			if event.pressed and is_finite(event.factor) and event.factor > 0:
				dragging = true
				var step: float = event.factor if event.button_index == MOUSE_BUTTON_WHEEL_UP else -event.factor
				if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
					rotate_drag(-deg_to_rad(10.0) * step)
				else:
					drag_depth = clampf(drag_depth + 0.25 * step, 1.0, 8.0)
					move_drag(drag_pointer)
			get_viewport().set_input_as_handled()
	elif event is InputEventMouseMotion and pressed != 0:
		# Captured motion belongs to mouse-look; keep the same screen-space grab.
		if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED: return
		drag_pointer = event.position
		if event.position.distance_to(press_position) > 6: dragging = true
		if dragging: move_drag(drag_pointer)
		get_viewport().set_input_as_handled()

func move_drag(position: Vector2) -> void:
	if not bindings.has(pressed): return
	var object: Node3D = bindings[pressed]
	var point := bounded_position(object, camera.project_position(position, drag_depth) + camera.global_basis * drag_offset)
	if placement_clear(object, point): object.global_position = point

func rotate_drag(angle: float) -> void:
	if not bindings.has(pressed): return
	var object: Node3D = bindings[pressed]
	var previous_basis := object.basis
	object.rotation.y = wrapf(object.rotation.y + angle, -PI, PI)
	# Rotate in place: reject blocked orientations instead of moving the panel
	# or changing its depth to fit the new bounds.
	if not object.global_position.is_equal_approx(bounded_position(object, object.global_position)) or not placement_clear(object, object.global_position):
		object.basis = previous_basis

func finish_drag(cancel: bool) -> void:
	if pressed == 0: return
	if cancel and bindings.has(pressed): bindings[pressed].global_transform = original_transform
	pressed = 0
	dragging = false
	if cancel: player.release_pointer()

func remove_object(window: int) -> void:
	if pressed == window: finish_drag(true)
	if selected == window: selected = 0
	bindings[window].queue_free()
	bindings.erase(window)

func clear_objects() -> void:
	for window in bindings.keys(): remove_object(window)

func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT: finish_drag(true)
