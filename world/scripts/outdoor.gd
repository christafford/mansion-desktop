## P22-T02 startup ground only; village landscape and architecture come later.
extends Node3D

# Insets keep whole panels inside the visible collision boundary and above ground.
const PLACEMENT := AABB(Vector3(-11.7, 0.08, -11.7), Vector3(23.4, 5.92, 23.4))
@onready var desktop_runtime: Node = $DesktopRuntime
var terminal_screen: Node:
	get: return desktop_runtime.terminal_screen
var application_mode: CanvasLayer:
	get: return desktop_runtime.application_mode
var app_launcher: CanvasLayer:
	get: return desktop_runtime.app_launcher
var application_objects: Node3D:
	get: return desktop_runtime.application_objects

static func bounded_center(center: Vector3, extents: Vector3) -> Vector3:
	return center.clamp(PLACEMENT.position + extents, PLACEMENT.end - extents)

func _ready() -> void:
	# Keep the existing selected-texture adapter without a second physical monitor.
	# The independent live panels remain the visible world presentation.
	$SelectedPreview/Screen.material_override = preload("res://scripts/screen_material.gd").create()
	desktop_runtime.initialize($Player, $SelectedPreview/Screen, $ApplicationStatus, bounded_center)
