## Approach from either side to open. Staying open avoids trapping carried objects.
extends Node3D

var opened := false
var progress := 0.0

func _physics_process(delta: float) -> void:
	if not opened:
		var player := get_tree().current_scene.get_node_or_null("Player") if get_tree().current_scene != null else null
		if player == null or player.application_mode: return
		var center: Vector3 = get_parent().global_position
		var offset: Vector3 = player.global_position - center
		offset.y = 0
		if offset.length() < 2.5: opened = true
	if opened and progress < 1.0:
		progress = minf(1.0, progress + delta / 0.45)
		rotation.y = -PI / 2 * smoothstep(0.0, 1.0, progress)
		if progress == 1.0: set_physics_process(false)
