## First-person study navigation, suspended while application mode owns input.
extends CharacterBody3D

const WALK_SPEED := 2.4
const LOOK_SENSITIVITY := 0.002
const TURN_SPEED := 1.8
const DEFAULT_PITCH := -0.08
const PITCH_RETURN_SECONDS := 0.2
const SPAWN := Vector3(2.4, 0.05, 3.1)
var slow_walk := false
var application_mode := false
var _held: Dictionary = {}
var _look_held := false
var _pitch_return: Tween
@onready var camera: Camera3D = $Camera3D

func _ready() -> void:
	return_to_spawn()

func return_to_spawn() -> void:
	_cancel_pitch_return()
	position = SPAWN
	rotation = Vector3(0, 0.38, 0)
	camera.rotation = Vector3(DEFAULT_PITCH, 0, 0)
	velocity = Vector3.ZERO

func release_pointer() -> void:
	_cancel_pitch_return()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	_look_held = false
	_held.clear()
	velocity = Vector3.ZERO

func _cancel_pitch_return() -> void:
	if _pitch_return != null:
		_pitch_return.kill()
		_pitch_return = null

func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT:
		release_pointer()

func _unhandled_input(event: InputEvent) -> void:
	if application_mode:
		return
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_RIGHT:
		_cancel_pitch_return()
		_look_held = event.pressed
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED if event.pressed else Input.MOUSE_MODE_VISIBLE
		if not event.pressed:
			_pitch_return = create_tween().set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
			_pitch_return.tween_property(camera, "rotation:x", DEFAULT_PITCH, PITCH_RETURN_SECONDS)
	if event is InputEventKey:
		if event.echo:
			return
		var key: int = event.physical_keycode if event.physical_keycode else event.keycode
		if event.pressed:
			_held[key] = true
		else:
			_held.erase(key)
		if event.pressed and key == KEY_ESCAPE:
			release_pointer()
		if event.pressed and key == KEY_HOME:
			return_to_spawn()
		if event.pressed and key == KEY_M:
			slow_walk = not slow_walk
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		rotation.y += event.screen_relative.x * LOOK_SENSITIVITY
		camera.rotation.x = clampf(camera.rotation.x - event.screen_relative.y * LOOK_SENSITIVITY, -1.35, 1.35)

func _physics_process(delta: float) -> void:
	if application_mode:
		velocity = Vector3.ZERO
		return
	var sideways := float(_held.has(KEY_D)) - float(_held.has(KEY_A))
	if _look_held:
		# While looking with the right button, A/D turn instead of strafing.
		rotation.y -= sideways * TURN_SPEED * delta
		sideways = 0.0
	var direction := Vector3(sideways, 0, float(_held.has(KEY_S)) - float(_held.has(KEY_W)))
	direction = basis * direction.normalized()
	var speed := 1.0 if slow_walk or _held.has(KEY_SHIFT) else WALK_SPEED
	velocity.x = direction.x * speed
	velocity.z = direction.z * speed
	if not is_on_floor():
		velocity.y -= 9.8 * delta
	else:
		velocity.y = 0
	move_and_slide()
