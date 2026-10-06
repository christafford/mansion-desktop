## First-person study navigation, suspended while application mode owns input.
extends CharacterBody3D

const WALK_SPEED := 2.4
const FAST_SPEED := 4.0
const SLOW_SPEED := 1.0
const LOOK_SENSITIVITY := 0.002
const DEFAULT_PITCH := -0.08
const SPAWN := Vector3(2.4, 0.05, 3.1)
var slow_walk := false
var application_mode := false
var _held: Dictionary = {}
var _look_held := false
var _host_focused := true
@onready var camera: Camera3D = $Camera3D

func _ready() -> void:
	return_to_spawn()

func return_to_spawn() -> void:
	release_pointer()
	position = SPAWN
	rotation = Vector3(0, 0.38, 0)
	camera.rotation = Vector3(DEFAULT_PITCH, 0, 0)

func stop_looking() -> void:
	# Releasing the mouse changes input ownership, never the viewing direction.
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	_look_held = false

func release_pointer() -> void:
	stop_looking()
	_held.clear()
	velocity = Vector3.ZERO

func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT:
		_host_focused = false
		release_pointer()
	elif what == NOTIFICATION_APPLICATION_FOCUS_IN:
		_host_focused = true

func _input(event: InputEvent) -> void:
	# GUI controls may consume releases; always end input we already own.
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_RIGHT and not event.pressed and _look_held:
		stop_looking()
		get_viewport().set_input_as_handled()
	elif event is InputEventKey and not event.pressed:
		var key: int = event.physical_keycode if event.physical_keycode else event.keycode
		_held.erase(key)

func _unhandled_input(event: InputEvent) -> void:
	if application_mode or not _host_focused:
		return
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_RIGHT:
		_look_held = event.pressed
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED if event.pressed else Input.MOUSE_MODE_VISIBLE
		get_viewport().set_input_as_handled()
	elif event is InputEventKey:
		if event.echo:
			return
		var key: int = event.physical_keycode if event.physical_keycode else event.keycode
		if key in [KEY_W, KEY_A, KEY_S, KEY_D, KEY_SHIFT, KEY_CTRL]:
			if event.pressed:
				_held[key] = true
			else:
				_held.erase(key)
		elif event.pressed and key == KEY_ESCAPE:
			release_pointer()
		elif event.pressed and key == KEY_HOME:
			return_to_spawn()
		elif event.pressed and key == KEY_M:
			slow_walk = not slow_walk
		else:
			return
		get_viewport().set_input_as_handled()
	elif event is InputEventMouseMotion and _look_held and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		rotation.y -= event.screen_relative.x * LOOK_SENSITIVITY
		camera.rotation.x = clampf(camera.rotation.x - event.screen_relative.y * LOOK_SENSITIVITY, -1.35, 1.35)
		get_viewport().set_input_as_handled()

func _physics_process(delta: float) -> void:
	if application_mode or not _host_focused:
		velocity = Vector3.ZERO
		return
	var direction := Vector3(float(_held.has(KEY_D)) - float(_held.has(KEY_A)), 0, float(_held.has(KEY_S)) - float(_held.has(KEY_W)))
	# Walk on the floor relative to yaw, with equal cardinal/diagonal speed.
	direction = basis * direction.normalized()
	var speed := WALK_SPEED
	if slow_walk or _held.has(KEY_CTRL):
		speed = SLOW_SPEED
	elif _held.has(KEY_SHIFT):
		speed = FAST_SPEED
	velocity.x = direction.x * speed
	velocity.z = direction.z * speed
	if not is_on_floor():
		velocity.y -= 9.8 * delta
	else:
		velocity.y = 0
	var walking_velocity := velocity * Vector3(1, 0, 1)
	move_and_slide()
	for i in range(get_slide_collision_count()):
		var contact := get_slide_collision(i)
		var body := contact.get_collider()
		if body is RigidBody3D and body.is_in_group("pushable_furniture"):
			body.push(walking_velocity, contact.get_normal(), delta)
