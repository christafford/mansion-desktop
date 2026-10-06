## A live presentation proxy: never moves the player or the physical world panel.
extends Node3D

const DURATION := 0.22
var app: CanvasLayer
var objects: Node3D
var camera: Camera3D
var surface: MeshInstance3D
var source: Node3D
var window := 0
var running := false
var entering := false
var elapsed := 0.0
var start := Transform3D.IDENTITY

func _ready() -> void:
	camera = objects.camera
	surface = MeshInstance3D.new()
	surface.mesh = QuadMesh.new()
	(surface.mesh as QuadMesh).size = Vector2.ONE
	surface.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	add_child(surface)
	surface.hide()

func world_pose() -> Transform3D:
	var size: Vector2 = source.screen.mesh.size
	return source.screen.global_transform * Transform3D(Basis.from_scale(Vector3(size.x, size.y, 1)), Vector3.ZERO)

func camera_pose() -> Transform3D:
	var rect: Rect2 = app.view.get_global_rect()
	var corner := camera.project_position(rect.position, 0.65)
	var across := camera.project_position(rect.position + Vector2(rect.size.x, 0), 0.65)
	var down := camera.project_position(rect.position + Vector2(0, rect.size.y), 0.65)
	return Transform3D(Basis(across - corner, corner - down, camera.global_basis.z), camera.project_position(rect.get_center(), 0.65))

func begin(handle: int, inward: bool) -> void:
	var reverse := running and window == handle and surface.visible
	var current := surface.global_transform
	cancel()
	window = handle
	entering = inward
	running = true
	elapsed = 0.0
	if bind_source():
		start = current if reverse else (world_pose() if entering else camera_pose())
		surface.global_transform = start

func bind_source() -> bool:
	source = objects.bindings.get(window)
	if not is_instance_valid(source) or not source.visible: return false
	# Share the shader/texture, including subsequent client commits and resizes.
	surface.material_override = source.screen.material_override
	source.set_presentation_hidden(true)
	surface.show()
	return true

func advance(delta: float) -> void:
	if not running: return
	if not is_instance_valid(source):
		# A newly launched window can map one frame before its world panel exists.
		if not bind_source():
			elapsed += delta
			if elapsed >= DURATION: complete()
			return
		start = world_pose() if entering else camera_pose()
		elapsed = 0.0
	if not source.visible:
		cancel()
		return
	elapsed = minf(elapsed + delta, DURATION)
	var t := elapsed / DURATION
	var eased := t * t * (3.0 - 2.0 * t)
	var target := camera_pose() if entering else world_pose()
	surface.global_transform = start.interpolate_with(target, eased)
	if elapsed >= DURATION: complete()

func complete() -> void:
	var show_application: bool = entering and app.active and app.focused_handle == window
	cancel()
	if show_application: app.panel.show()

func cancel() -> void:
	if is_instance_valid(source): source.set_presentation_hidden(false)
	source = null
	running = false
	surface.hide()
	surface.material_override = null
