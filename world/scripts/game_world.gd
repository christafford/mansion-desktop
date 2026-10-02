## Mansion Desktop — Game World controller
## P21-T01 scaffold: basic camera movement, collision bounds, spawn point.
## P21-T07: comfortable movement with collision, focus handling, reduced motion.
## P21-T08: stable entity IDs, ray picking, monitor screen slot.
## Full feature set (Godot input, bridge integration, furnished room)
## will be added incrementally through P21-T08, P21-T11-T19.

class_name GameWorld
extends Node3D

# ── Camera state ──────────────────────────────────────────────────
const CAMERA_HEIGHT := 1.6
const MOVE_SPEED_NORMAL := 3.0
const MOVE_SPEED_SLOW := 1.0
const LOOK_SENSITIVITY := 0.002

# ── Entity ID system (P21-T08) ───────────────────────────────────
## Stable entity IDs independent of node paths, PIDs, pointers, GPU objects
const ENTITY_ID_COUNTER_START := 1000
var _next_entity_id := ENTITY_ID_COUNTER_START
var _entity_registry := {}  # Maps entity_id -> node_path

# ── Monitor screen slot (P21-T08) ────────────────────────────────
var _monitor_slot_node: Node3D = null
var _monitor_screen_node: MeshInstance3D = null
var _desk_node: Node3D = null

# ── Picking state (P21-T08) ──────────────────────────────────────
var _pick_ray_origin := Vector3.ZERO
var _pick_ray_direction := Vector3.ZERO
var _pick_hit_position := Vector3.ZERO
var _pick_hit_normal := Vector3.ZERO
var _pick_hit_entity_id := 0

var _camera: Camera3D
var _spawn_position := Vector3.ZERO
var _spawn_rotation := Vector3.ZERO

# Input mode: "world" = camera moves, "app" = input forwarded to bridge
var input_mode := "world"  # "world" | "app"

# Movement state
var _reduced_motion := false

# ── Collision bounds (room interior) ──────────────────────────────
const ROOM_HALF := 4.5
const ROOM_HEIGHT := 6.5

# ── Teleport target ───────────────────────────────────────────────
const TELEPORT_POS := Vector3(0, 1.6, 0)
const TELEPORT_YAW := 0.0

# ── World key (default F12 = key code 88 in Godot) ────────────────
const WORLD_KEY := KEY_F12

# ── Reduced motion toggle (Shift+M) ───────────────────────────────
const TOGGLE_REDUCED_MOTION := KEY_M


func _enter_tree() -> void:
	# Resolve camera and spawn point references.
	_camera = _find_camera(self)
	if _camera:
		_spawn_position = _camera.position
		_spawn_rotation = _camera.rotation

	# Register monitor slot nodes (P21-T08)
	_register_monitor_slot()

	# Register global input mappings if not already present.
	_register_input()


# ── Public API ────────────────────────────────────────────────────

func get_spawn_position() -> Vector3:
	return _spawn_position


func get_spawn_rotation() -> Vector3:
	return _spawn_rotation


func get_camera() -> Camera3D:
	return _camera


func get_monitor_slot() -> Node3D:
	return _monitor_slot_node


func get_monitor_screen() -> MeshInstance3D:
	return _monitor_screen_node


func get_desk_node() -> Node3D:
	return _desk_node


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
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)


## Return to spawn position (safe spawn).
func return_to_spawn() -> void:
	if _camera:
		_camera.position = _spawn_position
		_camera.rotation = _spawn_rotation


## Toggle reduced motion mode.
func toggle_reduced_motion() -> void:
	_reduced_motion = not _reduced_motion


## Get current movement speed based on reduced motion setting.
func get_move_speed() -> float:
	if _reduced_motion:
		return MOVE_SPEED_SLOW
	return MOVE_SPEED_NORMAL


## Teleport the camera to the monitor slot position.
func teleport_to_monitor() -> void:
	if _camera:
		_camera.position = TELEPORT_POS
		_camera.rotation = Vector3(0, TELEPORT_YAW, 0)
		_spawn_position = _camera.position


## ── Entity ID System (P21-T08) ────────────────────────────────────

## Assign a stable entity ID for a node.
func assign_entity_id(node_path: NodePath) -> int:
	_next_entity_id += 1
	_entity_registry[_next_entity_id] = node_path
	return _next_entity_id


## Get node path for an entity ID, or null if not found.
func get_entity_path(entity_id: int) -> NodePath:
	if entity_id in _entity_registry:
		return _entity_registry[entity_id]
	return NodePath("")


## Get entity ID for a node path, or 0 if not found.
func get_entity_id_for_path(node_path: NodePath) -> int:
	for eid in _entity_registry:
		if _entity_registry[eid] == node_path:
			return eid
	return 0


## Remove an entity from the registry.
func remove_entity_id(entity_id: int) -> void:
	if entity_id in _entity_registry:
		_entity_registry.erase(entity_id)


## ── Monitor Screen Slot (P21-T08) ────────────────────────────────

## Find and register monitor slot and desk nodes.
func _register_monitor_slot() -> void:
	_monitor_slot_node = _find_node_by_name(self, "MonitorSlot") as Node3D
	_monitor_screen_node = _find_node_by_name(self, "MonitorScreen") as MeshInstance3D
	_desk_node = _find_node_by_name(self, "Desk") as Node3D
	
	if _monitor_slot_node and _monitor_screen_node:
		# Assign stable entity IDs
		assign_entity_id(get_path_to(_monitor_screen_node))
		assign_entity_id(get_path_to(_monitor_slot_node))
		
		# Parent the diagnostic screen elements to the main screen
		# This ensures they move/rotate with the desk when parented
		pass  # Nodes are already in proper hierarchy in scene file


## Get the monitor screen's aspect ratio.
func get_monitor_aspect_ratio() -> float:
	# Monitor frame is 1.6 x 0.9 in world units
	return 1.6 / 0.9


## Get the monitor screen's position in world space.
func get_monitor_screen_position() -> Vector3:
	if _monitor_screen_node:
		return _monitor_screen_node.global_position
	return Vector3.ZERO


## Get the monitor screen's rotation in world space.
func get_monitor_screen_rotation() -> Quaternion:
	if _monitor_screen_node:
		return _monitor_screen_node.global_transform.basis.get_rotation_quaternion()
	return Quaternion.IDENTITY


## ── Debug/Verification (P21-T08) ─────────────────────────────────


## Exported debug function: return current entity registry.
func get_entity_registry() -> Dictionary:
	return _entity_registry


## Exported debug function: return pick state.
func get_pick_state() -> Dictionary:
	return {
		"ray_origin": _pick_ray_origin,
		"ray_direction": _pick_ray_direction,
		"hit_position": _pick_hit_position,
		"hit_normal": _pick_hit_normal,
		"hit_entity_id": _pick_hit_entity_id
	}

## Cast a ray from camera through screen point and return hit info.
func ray_pick(screen_point: Vector2, max_distance: float = 10.0) -> Dictionary:
	if not _camera:
		return {"hit": false, "position": Vector3.ZERO, "normal": Vector3.ZERO, "entity_id": 0}
	
	# Get ray origin (camera position) and direction
	var ray_origin := _camera.global_position
	var ray_direction := _camera.project_ray_normal(screen_point)
	
	# Store for reference
	_pick_ray_origin = ray_origin
	_pick_ray_direction = ray_direction
	
	# Simple box intersection test for furniture and monitor screen
	var closest_hit := -1.0
	var hit_position := Vector3.ZERO
	var hit_normal := Vector3.ZERO
	var hit_entity_id := 0
	
	# Test monitor screen (approximate plane at monitor slot)
	if _monitor_screen_node:
		var screen_pos := _monitor_screen_node.global_position
		var screen_normal := _monitor_screen_node.global_transform.basis.z.normalized()
		
		# Ray-plane intersection
		var denom := ray_direction.dot(screen_normal)
		if abs(denom) > 0.0001:
			var to_plane := screen_pos - ray_origin
			var distance := to_plane.dot(screen_normal) / denom
			if distance >= 0 and distance < max_distance:
				hit_position = ray_origin + ray_direction * distance
				hit_normal = screen_normal
				hit_entity_id = get_entity_id_for_path(get_path_to(_monitor_screen_node))
				closest_hit = distance
	
	# Test furniture bounding boxes
	var furniture_centers := [
		Vector3(0.0, 0.8, 2.0),   # Desk
		Vector3(0.0, 0.8, -2.0),  # Chair
		Vector3(-2.5, 2.5, 0.0),  # Bookshelf
		Vector3(0.5, 0.5, 1.5),   # Bookset
		Vector3(-0.5, 1.2, 1.5),  # Lamp
		Vector3(2.5, 0.5, 0.0),   # Plant
	]
	
	for center in furniture_centers:
		var half_size := Vector3(1.0, 1.0, 1.0)
		var hit := _ray_box_intersection(ray_origin, ray_direction, center, half_size, max_distance)
		if hit["hit"] and (closest_hit < 0 or hit["distance"] < closest_hit):
			closest_hit = hit["distance"]
			hit_position = hit["position"]
			hit_normal = hit["normal"]
			hit_entity_id = get_entity_id_for_path(NodePath("Room/" + center.to_string()))
	
	# Store results
	_pick_hit_position = hit_position
	_pick_hit_normal = hit_normal
	_pick_hit_entity_id = hit_entity_id
	
	if closest_hit >= 0:
		return {
			"hit": true,
			"position": hit_position,
			"normal": hit_normal,
			"entity_id": hit_entity_id,
			"distance": closest_hit
		}
	
	return {"hit": false, "position": Vector3.ZERO, "normal": Vector3.ZERO, "entity_id": 0}


## Helper: ray-box intersection test.
func _ray_box_intersection(ray_origin: Vector3, ray_dir: Vector3, box_center: Vector3, box_half: Vector3, max_dist: float) -> Dictionary:
	# Inverse direction
	var inv_dir := Vector3.ZERO
	if abs(ray_dir.x) > 0.0001:
		inv_dir.x = 1.0 / ray_dir.x
	else:
		inv_dir.x = 1e10
	if abs(ray_dir.y) > 0.0001:
		inv_dir.y = 1.0 / ray_dir.y
	else:
		inv_dir.y = 1e10
	if abs(ray_dir.z) > 0.0001:
		inv_dir.z = 1.0 / ray_dir.z
	else:
		inv_dir.z = 1e10
	
	# Slab test
	var t1 := (box_center.x - box_half.x - ray_origin.x) * inv_dir.x
	var t2 := (box_center.x + box_half.x - ray_origin.x) * inv_dir.x
	var t3 := (box_center.y - box_half.y - ray_origin.y) * inv_dir.y
	var t4 := (box_center.y + box_half.y - ray_origin.y) * inv_dir.y
	var t5 := (box_center.z - box_half.z - ray_origin.z) * inv_dir.z
	var t6 := (box_center.z + box_half.z - ray_origin.z) * inv_dir.z
	
	# Explicitly typed floats
	var tmin: float = max(max(min(t1, t2), min(t3, t4)), min(t5, t6))
	var tmax: float = min(min(max(t1, t2), max(t3, t4)), max(t5, t6))
	
	# If tmin > tmax, ray doesn't intersect box
	if tmin > tmax or tmax < 0:
		return {"hit": false, "position": Vector3.ZERO, "normal": Vector3.ZERO}
	
	# Hit position (explicitly typed)
	var hit_pos: Vector3 = ray_origin + ray_dir * tmin
	if tmin < 0:
		hit_pos = ray_origin + ray_dir * tmax
	
	# Check if inside box
	if (abs(hit_pos.x - box_center.x) <= box_half.x and
	    abs(hit_pos.y - box_center.y) <= box_half.y and
	    abs(hit_pos.z - box_center.z) <= box_half.z):
		# Calculate normal (simplified - points toward ray origin) - explicitly typed
		var normal: Vector3 = box_center - hit_pos
		normal = normal.normalized()
		return {"hit": true, "position": hit_pos, "normal": normal, "distance": tmin if tmin >= 0 else tmax}
	
	return {"hit": false, "position": Vector3.ZERO, "normal": Vector3.ZERO}


## Get the last ray pick result.
func get_last_pick_result() -> Dictionary:
	return {
		"origin": _pick_ray_origin,
		"direction": _pick_ray_direction,
		"hit_position": _pick_hit_position,
		"hit_normal": _pick_hit_normal,
		"hit_entity_id": _pick_hit_entity_id
	}


## Clamp camera position inside collision bounds.
func clamp_to_room(pos: Vector3) -> Vector3:
	pos.x = clampf(pos.x, -ROOM_HALF, ROOM_HALF)
	pos.y = clampf(pos.y, CAMERA_HEIGHT, ROOM_HEIGHT)
	pos.z = clampf(pos.z, -ROOM_HALF, ROOM_HALF)
	return pos


## Check collision with furniture (basic box collision).
func check_furniture_collision(pos: Vector3, radius: float = 0.5) -> bool:
	# Furniture bounding boxes (approximate world-space positions)
	# Desk: near +Z, chair: near -Z, bookshelf: near -X, plant: near +X
	# These are approximate and should be refined with actual node positions
	
	# Desk area (metal_office_desk at approximately +Z)
	var desk_center := Vector3(0.0, 0.8, 2.0)
	var desk_half := Vector3(1.0, 1.6, 1.0)
	if _check_box_collision(pos, desk_center, desk_half, radius):
		return true
	
	# Chair area (dining_chair_02 at approximately -Z)
	var chair_center := Vector3(0.0, 0.8, -2.0)
	var chair_half := Vector3(0.5, 1.0, 0.5)
	if _check_box_collision(pos, chair_center, chair_half, radius):
		return true
	
	# Bookshelf area (wooden_bookshelf_worn at approximately -X)
	var bookshelf_center := Vector3(-2.5, 2.5, 0.0)
	var bookshelf_half := Vector3(0.5, 2.5, 1.0)
	if _check_box_collision(pos, bookshelf_center, bookshelf_half, radius):
		return true
	
	# Bookset area (book_encyclopedia_set_01 near desk)
	var bookset_center := Vector3(0.5, 0.5, 1.5)
	var bookset_half := Vector3(0.3, 0.5, 0.3)
	if _check_box_collision(pos, bookset_center, bookset_half, radius):
		return true
	
	# Lamp area (desk_lamp_arm_01 near desk)
	var lamp_center := Vector3(-0.5, 1.2, 1.5)
	var lamp_half := Vector3(0.3, 0.8, 0.3)
	if _check_box_collision(pos, lamp_center, lamp_half, radius):
		return true
	
	# Plant area (potted_plant_02 near +X)
	var plant_center := Vector3(2.5, 0.5, 0.0)
	var plant_half := Vector3(0.4, 0.5, 0.4)
	if _check_box_collision(pos, plant_center, plant_half, radius):
		return true
	
	return false


## Helper: check collision with a box.
func _check_box_collision(pos: Vector3, box_center: Vector3, box_half: Vector3, radius: float) -> bool:
	var dx: float = abs(pos.x - box_center.x)
	var dy: float = abs(pos.y - box_center.y)
	var dz: float = abs(pos.z - box_center.z)
	
	return dx < (box_half.x + radius) and dy < (box_half.y + radius) and dz < (box_half.z + radius)


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
		move = move.normalized() * get_move_speed() * delta
		# Move relative to camera yaw (forward = -Z in camera space).
		var forward := Vector3(0, 0, -1)
		forward = forward.rotated(Vector3.UP, _camera.rotation.y)
		var right := Vector3(1, 0, 0)
		right = right.rotated(Vector3.UP, _camera.rotation.y)

		var next_pos := _camera.position + forward * (-move.z) + right * move.x
		# Check furniture collision before applying
		if not check_furniture_collision(next_pos):
			_camera.position = next_pos
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

	# Reduced motion toggle: Shift+M
	if event is InputEventKey and event.keycode == TOGGLE_REDUCED_MOTION and event.pressed:
		if Input.is_key_pressed(KEY_SHIFT):
			toggle_reduced_motion()
			print("Reduced motion: " + str(_reduced_motion))
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


## Find a node by name in the scene tree (BFS).
func _find_node_by_name(root: Node, name: String) -> Node:
	if root.name == name:
		return root
	for child in root.get_children():
		var found := _find_node_by_name(child, name)
		if found:
			return found
	return null


## Window focus lost notification constant (Godot 4.x)
const NOTIFICATION_WM_FOCUS_LOST := 234

func _notification(what: int) -> void:
	# Handle focus loss - release mouse capture when window loses focus
	if what == NOTIFICATION_WM_FOCUS_LOST:
		if input_mode == "world" and Input.get_mouse_mode() == Input.MOUSE_MODE_CAPTURED:
			Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
