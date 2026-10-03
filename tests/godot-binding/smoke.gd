extends SceneTree

func _initialize() -> void:
	if not ClassDB.class_exists("MansionBindingProbe"):
		push_error("Native probe class was not registered")
		quit(1)
		return
	for iteration in range(100):
		var probe = ClassDB.instantiate("MansionBindingProbe")
		if probe == null or probe.answer() != 42:
			push_error("Native probe call returned the wrong answer")
			quit(1)
			return
		probe = null
	print("BINDING_SMOKE_OK: 100 native instances/calls/releases")
	quit(0)
