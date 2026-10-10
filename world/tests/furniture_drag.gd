## Injected shared drag controls and real dynamic furniture, no synthetic clients.
extends "res://tests/application_objects.gd"

func run() -> void:
	study = load("res://scenes/study.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	objects = study.application_objects
	var player = study.get_node("Player")
	player.set_physics_process(false)
	root.grab_focus()
	await settle()
	for name in ["Chair", "Plant", "Bookshelf"]:
		var body = study.get_node(name)
		var original: Transform3D = body.global_transform
		player.position = Vector3(3, 0.05, 3)
		await place(body, Transform3D(Basis.IDENTITY, Vector3(0, 0.1, 1)))
		for frame in range(45): await physics_frame
		player.position = Vector3(0, 0.05, 3.4)
		player.rotation = Vector3.ZERO
		player.camera.look_at(body.to_global(body.placement_box.get_center()))
		await settle()
		var pointer: Vector2 = objects.camera.unproject_position(body.to_global(body.placement_box.get_center()))
		check(objects.pick_furniture(pointer) == body, name + " is not pickable")
		var before: Transform3D = body.global_transform
		await mouse(pointer, true)
		check(objects.furniture == body and body.freeze, name + " did not grab/freeze")
		await motion(pointer + Vector2(70, -100))
		check(body.position.distance_to(before.origin) > 0.2, name + " did not move")
		var carried: Vector3 = body.position
		player.set_physics_process(true)
		await key(KEY_W, true)
		for frame in range(8): await physics_frame
		await key(KEY_W, false)
		player.set_physics_process(false)
		check(body.position.distance_to(carried) > 0.15, name + " cannot be carried while walking")
		check(not objects.placement_clear(body, study.get_node("Desk").position), name + " can overlap desk")
		var moved: Vector3 = body.position
		await mouse(pointer, true, false, MOUSE_BUTTON_WHEEL_UP)
		check(body.position.distance_to(moved) > 0.1, name + " ignores depth wheel")
		await mouse(pointer, true, false, MOUSE_BUTTON_RIGHT)
		await mouse(pointer, true, false, MOUSE_BUTTON_WHEEL_UP)
		check(absf(body.rotation.y) > 0.1, name + " ignores rotation wheel")
		await mouse(pointer, false, false, MOUSE_BUTTON_RIGHT)
		await tap(KEY_ESCAPE)
		await mouse(pointer, false)
		check(body.position.distance_to(before.origin) < 0.025 and body.basis.get_rotation_quaternion().angle_to(before.basis.get_rotation_quaternion()) < 0.02 and not body.freeze and not objects.has_grab(), name + " cancel lost original pose/unfreeze: before=%s after=%s frozen=%s grab=%s" % [before, body.global_transform, body.freeze, objects.has_grab()])
		for cause in ["release", "launcher", "focus"]:
			pointer = objects.camera.unproject_position(body.to_global(body.placement_box.get_center()))
			await mouse(pointer, true)
			await motion(pointer + Vector2(40, -90))
			check(body.freeze and objects.has_grab(), name + " subsequent grab failed: cause=%s body=%s pick=%s pointer=%s" % [cause, body.position, objects.pick_furniture(pointer), pointer])
			if cause == "launcher":
				study.app_launcher.open()
				await settle()
				study.app_launcher.close()
			elif cause == "focus": objects._notification(Node.NOTIFICATION_APPLICATION_FOCUS_OUT)
			await mouse(pointer, false)
			check(not body.freeze and not objects.has_grab(), name + " remains frozen after " + cause)
			for frame in range(50): await physics_frame
			check(absf(body.position.y) < 0.035, name + " did not land on floor")
		# Actual controller walking contact, away from other room furniture.
		player.position = body.position + Vector3(0, 0.05, 1.2)
		player.rotation = Vector3.ZERO
		player.camera.rotation = Vector3(-0.15, 0, 0)
		var start: Vector3 = body.position
		player.set_physics_process(true)
		await key(KEY_W, true)
		for frame in range(55): await physics_frame
		await key(KEY_W, false)
		player.set_physics_process(false)
		check(body.position.distance_to(start) > 0.12, name + " does not yield to walking")
		check(absf(body.rotation.x) < 0.01 and absf(body.rotation.z) < 0.01, name + " tipped over")
		player.position = Vector3(3, 0.05, 3)
		await place(body, original)
	await terminal.shutdown()
	print("FURNITURE_DRAG_OK items=3 failures=", failures)
	quit(1 if failures else 0)

func place(body: RigidBody3D, pose: Transform3D) -> void:
	body.freeze = true
	for frame in range(3): await physics_frame
	body.global_transform = pose
	body.linear_velocity = Vector3.ZERO
	body.angular_velocity = Vector3.ZERO
	for frame in range(3): await physics_frame
	body.freeze = false
	body.sleeping = false
