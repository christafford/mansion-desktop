## Mansion Desktop — Game World controller
## P21-T01 scaffold: basic camera movement, collision bounds, spawn point.
## Full feature set (Godot input, bridge integration, furnished room)
## will be added incrementally through P21-T07, P21-T11-T19.

class_name GameWorld
extends Node3D

# ── Camera state ──────────────────────────────────────────────────
const CAMERA_HEIGHT := 1.6
const MOVE_SPEED := 3.0
const LOOK_SENSITIVITY := 0.002

var _camera: Camera3D
var _spawn_position := Vector3.ZERO
var _spawn_rotation := Vector3.ZERO

# Input mode: "world" = camera moves, "app" = input forwarded to bridge
var input_mode := "world"  # "world" | "app"

# ── Collision bounds (room interior) ──────────────────────────────
const ROOM_HALF := 4.5
const ROOM_HEIGHT := 6.5

# ── Teleport target ───────────────────────────────────────────────
const TELEPORT_POS := Vector3(0, 1.6, 0)
const TELEPORT_YAW := 0.0

# ── World key (default F12 = key code 88 in Godot) ────────────────
const WORLD_KEY := KEY_F12


func _enter_tree() -> void:
	# Resolve camera and spawn point references.
	_camera = _find_camera(self)
	if _camera:
		_spawn_position = _camera.position
		_spawn_rotation = _camera.rotation

	# Register global input mappings if not already present.
	_register_input()


# ── Public API ────────────────────────────────────────────────────

func get_spawn_position() -> Vector3:
	return _spawn_position


func get_spawn_rotation() -> Vector3:
	return _spawn_rotation


func get_camera() -> Camera3D:
	return _camera


func get_input_mode() -> String:
	return input_mode


## Switch to application mode: consume camera input,
## forward keyboard/pointer to the GDExtension bridge.
func enter_application_mode() -> void:
	input_mode = "app"
	if _camera:
		_camera.make_current()


## Return to world mode: reclaim camera input, release held keys.
func exit_application_mode() -> void:
	input_mode = "world"


## Teleport the camera to the monitor slot position.
func teleport_to_monitor() -> void:
	if _camera:
		_camera.position = TELEPORT_POS
		_camera.rotation = Vector3(0, TELEPORT_YAW, 0)
		_spawn_position = _camera.position


## Clamp camera position inside collision bounds.
func clamp_to_room(pos: Vector3) -> Vector3:
	pos.x = clampf(pos.x, -ROOM_HALF, ROOM_HALF)
	pos.y = clampf(pos.y, CAMERA_HEIGHT, ROOM_HEIGHT)
	pos.z = clampf(pos.z, -ROOM_HALF, ROOM_HALF)
	return pos


# ── Input handling ────────────────────────────────────────────────

func _process(delta: float) -> void:
	if input_mode != "world":
		return
	if not _camera:
		return

	var move := Vector3.ZERO

	if Input.is_key_pressed(KEY_W) or Input.is_key_pressed(KEY_UP):
		move.z -= 1.0
	if Input.is_key_pressed(KEY_S) or Input.is_key_pressed(KEY_DOWN):
		move.z += 1.0
	if Input.is_key_pressed(KEY_A) or Input.is_key_pressed(KEY_LEFT):
		move.x -= 1.0
	if Input.is_key_pressed(KEY_D) or Input.is_key_pressed(KEY_RIGHT):
		move.x += 1.0

	if move.length() > 0:
		move = move.normalized() * MOVE_SPEED * delta
		# Move relative to camera yaw (forward = -Z in camera space).
		var forward := Vector3(0, 0, -1)
		forward = forward.rotated(Vector3.UP, _camera.rotation.y)
		var right := Vector3(1, 0, 0)
		right = right.rotated(Vector3.UP, _camera.rotation.y)

		_camera.position += forward * (-move.z) + right * move.x
		_camera.position.y = CAMERA_HEIGHT  # Keep at walking height
		_camera.position = clamp_to_room(_camera.position)


func _input(event: InputEvent) -> void:
	# World key always exits application mode.
	if event is InputEventKey and event.keycode == WORLD_KEY:
		if event.pressed and input_mode == "app":
			exit_application_mode()
			return

	# Teleport: T key (Godot keycode for T is KEY_T).
	if event is InputEventKey and event.keycode == KEY_T and event.pressed:
		teleport_to_monitor()
		return

	# Enter application mode: Enter key from world mode.
	if event is InputEventKey and event.keycode == KEY_ENTER and event.pressed:
		if input_mode == "world":
			enter_application_mode()
			return

	# Mouse look (right mouse button held).
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_RIGHT:
			Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
		elif event.button_index == MOUSE_BUTTON_RIGHT and not event.pressed:
			Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)

	if event is InputEventMouseMotion and Input.get_mouse_mode() == Input.MOUSE_MODE_CAPTURED:
		if input_mode == "world" and _camera:
			_camera.rotation.y -= event.relative_x * LOOK_SENSITIVITY
			_camera.rotation.x -= event.relative_y * LOOK_SENSITIVITY
			_camera.rotation.x = clampf(_camera.rotation.x, -PI / 2.1, PI / 2.1)


# ── Helpers ───────────────────────────────────────────────────────

func _register_input() -> void:
	# Register movement actions so Godot's input system recognises them.
	# These are mostly for documentation; we also check raw keycodes.
	if not InputMap.has_action("gw_move_forward"):
		InputMap.add_action("gw_move_forward")
	if not InputMap.has_action("gw_move_back"):
		InputMap.add_action("gw_move_back")
	if not InputMap.has_action("gw_move_left"):
		InputMap.add_action("gw_move_left")
	if not InputMap.has_action("gw_move_right"):
		InputMap.add_action("gw_move_right")
	if not InputMap.has_action("gw_teleport"):
		InputMap.add_action("gw_teleport")
	if not InputMap.has_action("gw_enter_app"):
		InputMap.add_action("gw_enter_app")
	if not InputMap.has_action("gw_exit_app"):
		InputMap.add_action("gw_exit_app")


func _find_camera(node: Node) -> Camera3D:
	if node is Camera3D:
		return node as Camera3D
	for child in node.get_children():
		var found := _find_camera(child)
		if found:
			return found
	return null
