## GPU views and actual WASD/RMB stair traversal with a live Wayland terminal.
extends "res://tests/mansion_rooms.gd"

const Foyer = preload("res://scripts/grand_foyer.gd")
var walker: CharacterBody3D
var foyer: Node3D

func steer(target: Vector3) -> void:
	var delta := target - walker.global_position
	var yaw := atan2(-delta.x, -delta.z)
	var look := InputEventMouseMotion.new()
	look.screen_relative = Vector2(-wrapf(yaw - walker.rotation.y, -PI, PI) / walker.LOOK_SENSITIVITY, 0)
	Input.parse_input_event(look)
	await process_frame

func go(local_target: Vector3) -> bool:
	var target := foyer.to_global(local_target)
	var deadline := Time.get_ticks_msec() + 5500
	await steer(target)
	await key(KEY_W, true)
	while Vector2(walker.position.x-target.x,walker.position.z-target.z).length() > 0.085 and Time.get_ticks_msec() < deadline:
		await steer(target)
		await physics_frame
	await key(KEY_W, false)
	var reached := Vector2(walker.position.x-target.x,walker.position.z-target.z).length() < 0.12
	if not reached:
		for i in range(walker.get_slide_collision_count()): print("BLOCKING_COLLIDER ",walker.get_slide_collision(i).get_collider().get_path())
	check(reached, "Walking blocked: target=%s actual=%s" % [local_target,foyer.to_local(walker.position)])
	return reached

func stairs(up: bool) -> bool:
	for i in range(101):
		var fraction := i/100.0 if up else 1.0-i/100.0
		var angle := lerpf(Foyer.STAIR_START-0.2,Foyer.STAIR_START+Foyer.STAIR_SWEEP,fraction)
		if not await go(Foyer.stair_point(angle,2.75,0)): return false
		check(absf(walker.position.y-Foyer.stair_height(angle)) < 0.23, "Stair floor lost at angle %s: y=%s" % [angle,walker.position.y])
		if up and i == 50:
			for radius in [5.0,0.5]:
				await steer(foyer.to_global(Foyer.stair_point(angle,radius,0)))
				await key(KEY_W,true)
				for frame in range(55): await physics_frame
				await key(KEY_W,false)
				var offset := foyer.to_local(walker.position)-Foyer.STAIR_CENTER
				var actual_radius := Vector2(offset.x,offset.z).length()
				check(actual_radius < Foyer.STAIR_OUTER-0.2 and actual_radius > Foyer.STAIR_INNER+0.2,"Spiral guard allowed passage: "+str(actual_radius))
				check(absf(walker.position.y-Foyer.stair_height(angle))<0.23,"Spiral guard allowed a fall")
				if not await go(Foyer.stair_point(angle,2.75,0)): return false
	return true

func run() -> void:
	if DisplayServer.get_name() == "headless":
		push_error("Foyer acceptance requires a graphical renderer")
		quit(1)
		return
	output_dir = ProjectSettings.globalize_path("res://../.tools/grand-foyer-test")
	DirAccess.make_dir_recursive_absolute(output_dir)
	study = load("res://scenes/main.tscn").instantiate()
	root.add_child(study)
	current_scene = study
	terminal = study.terminal_screen
	app = study.application_mode
	objects = study.application_objects
	walker = study.get_node("Player")
	foyer = study.get_node("Hallway/Rooms/foyer")
	var deadline := Time.get_ticks_msec()+15000
	while objects.bindings.is_empty() and Time.get_ticks_msec()<deadline: await process_frame
	root.grab_focus()
	await settle()
	check(not objects.bindings.is_empty() and app.host_focused,"Missing live client/host focus")
	if failures:
		await terminal.shutdown()
		quit(1)
		return
	# Fixed eye-level and gallery captures; no synthetic application imagery.
	walker.set_physics_process(false)
	var views := [
		["entrance",Vector3(0,1.65,1),Vector3(0,5.4,12)],
		["spiral",Vector3(-5.5,1.65,7),Vector3(0,4.5,12)],
		["upper-gallery",Vector3(7.85,7.2,15),Vector3(-1,4.2,11)],
		["return",Vector3(0,1.65,22),Vector3(0,4.0,5)]
	]
	for view in views:
		walker.camera.global_position=foyer.to_global(view[1])
		walker.camera.look_at(foyer.to_global(view[2]))
		await capture(view[0]+".png")
		await profile_view("foyer-"+view[0])
		print("FOYER_RENDER_COST ",view[0]," draws=",Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME)," objects=",Performance.get_monitor(Performance.RENDER_TOTAL_OBJECTS_IN_FRAME)," primitives=",Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME))
	if "--foyer-captures-only" in OS.get_cmdline_user_args():
		await terminal.shutdown()
		print("FOYER_RENDERED failures=",failures)
		quit(1 if failures else 0)
		return
	walker.camera.position=Vector3(0,1.6,0)
	walker.camera.rotation=Vector3.ZERO
	walker.position=foyer.to_global(Vector3(0,0.05,-2))
	walker.rotation=Vector3(0,PI,0)
	walker.set_physics_process(true)
	await mouse(Vector2(640,400),true,false,MOUSE_BUTTON_RIGHT)
	check(await go(Vector3(0,0,6)),"Cannot enter foyer from hall")
	check(absf(walker.position.y)<0.04,"Entry floor discontinuity")
	# Carry the live terminal from the ground floor around all 40 risers.
	check(await go(Foyer.stair_point(Foyer.STAIR_START-0.2,2.75,0)),"Cannot approach stair")
	var window: int=terminal.handle
	var panel=objects.bindings[window]
	var identity: String=panel.entity_id
	await steer(foyer.to_global(Foyer.stair_point(Foyer.STAIR_START+0.1,2.75,0)))
	panel.gravity_active=false
	panel.position=walker.camera.to_global(Vector3(0,0,-1.05))
	panel.rotation.y=walker.rotation.y
	await mouse(Vector2(640,400),false,false,MOUSE_BUTTON_RIGHT)
	await settle()
	await mouse(point(window),true)
	check(objects.pressed==window,"Cannot pick up live terminal at stair")
	await mouse(point(window),true,false,MOUSE_BUTTON_RIGHT)
	var ascended := await stairs(true)
	if ascended:
		check(await go(Vector3(2.75,5.6,12.7)),"Cannot step onto landing")
		check(await go(Vector3(8.5,5.6,12.7)),"Cannot cross gallery bridge")
		check(absf(walker.position.y-5.6)<0.04,"Upper gallery is not supporting walker")
		check(panel.position.y>6 and panel.position.distance_to(walker.camera.global_position)<2,"Carried terminal did not reach gallery")
		await mouse(point(window),false)
		await mouse(point(window),false,false,MOUSE_BUTTON_RIGHT)
		for frame in range(80): await physics_frame
		check(absf(panel.position.y-panel.bounds.y/2-5.6)<0.04,"Terminal failed to land on gallery: "+str(panel.position))
		check(panel.entity_id==identity,"Stair carrying changed application identity")
		walker.camera.look_at(panel.position)
		await activate(window)
		var marker := output_dir.path_join("typed.txt")
		if FileAccess.file_exists(marker): DirAccess.remove_absolute(marker)
		await type_text("printf 'FOYER GALLERY' > '"+marker.replace("'","'\"'\"'")+"'\n")
		await settle()
		check(FileAccess.file_exists(marker) and FileAccess.get_file_as_string(marker)=="FOYER GALLERY","Upper gallery terminal lost real typing")
		await return_to_room()
		await capture("gallery-application.png")
		# Walk into the side gallery guard; do not fall to the floor below.
		walker.camera.rotation=Vector3.ZERO
		await mouse(Vector2(640,400),true,false,MOUSE_BUTTON_RIGHT)
		await go(Vector3(8.5,5.6,15))
		await steer(foyer.to_global(Vector3(0,5.6,15)))
		await key(KEY_W,true)
		for frame in range(75): await physics_frame
		await key(KEY_W,false)
		check(walker.position.x>7.5 and absf(walker.position.y-5.6)<0.04,"Gallery guard allowed a fall")
		await go(Vector3(8.5,5.6,12.7))
		await go(Vector3(2.75,5.6,12.7))
		check(await stairs(false),"Cannot descend spiral staircase")
		for frame in range(10): await physics_frame
		check(absf(walker.position.y)<0.04,"Stair descent did not reach floor")
	# A dropped panel uses the real flat treads, not the character's smooth ramp.
	await mouse(Vector2(640,400),false)
	await mouse(Vector2(640,400),false,false,MOUSE_BUTTON_RIGHT)
	walker.position=foyer.to_global(Vector3(-5,0.05,9))
	var angle := Foyer.STAIR_START+Foyer.STAIR_SWEEP*5.5/Foyer.STEPS
	panel.position=foyer.to_global(Foyer.stair_point(angle,2.75,2.1))
	panel.rotation.y=-angle
	panel.gravity_active=true
	panel.fall_speed=0
	for frame in range(90): await physics_frame
	check(absf(panel.position.y-panel.bounds.y/2-6*Foyer.RISE)<0.04,"Dropped application missed flat stair tread: "+str(panel.position))
	await terminal.shutdown()
	print("GRAND_FOYER_OK levels=2 risers=40 failures=",failures)
	quit(1 if failures else 0)
