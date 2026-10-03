## Checks actual client bytes through the binding; not rendered-image acceptance.
extends SceneTree
var session
var pid := -1
var failures := 0
var ticks := 0
var old_handle := 0
var retained: Dictionary = {}
const ARGB := [255,0,0,255, 0,255,0,255, 0,0,255,255, 128,64,32,128,
	0,0,0,0, 255,255,255,255, 18,52,86,255, 171,205,239,255]
const XRGB := [10,20,30,255, 40,50,60,255, 70,80,90,255,
	100,110,120,255, 130,140,150,255, 160,170,180,255]
const STAGES := ["initial", "destroyed", "replacement", "pending", "scaled", "pending-destroyed", "detached", "remapped",
	"transform-0", "transform-1", "transform-2", "transform-3", "transform-4", "transform-5", "transform-6", "transform-7"]

func check(ok: bool, message: String) -> bool:
	if not ok:
		push_error(message)
		failures += 1
	return ok

func tick() -> void:
	await process_frame
	ticks += 1
	check(session.pump(), session.last_error())

func wait_marker(path: String) -> bool:
	var deadline := Time.get_ticks_msec() + 5000
	while not FileAccess.file_exists(path) and Time.get_ticks_msec() < deadline:
		await tick()
	return check(FileAccess.file_exists(path), "Missing client marker " + path)

func verify(f: Dictionary, stage: String) -> void:
	if not check(not f.is_empty(), "Missing frame"):
		return
	var xrgb := stage in ["replacement", "pending"]
	var detached := stage == "detached"
	var scaled := stage in ["scaled", "pending-destroyed", "detached"]
	var transform := int(stage.right(1)) if stage.begins_with("transform-") else (1 if scaled else 0)
	var revision := 1 if stage in ["initial", "destroyed"] else (2 if xrgb else ((4 if detached else 3) if scaled else (6 if stage == "remapped" else 6 + transform)))
	check(f.revision == revision, "Revision for " + stage)
	check(f.mapped == not detached, "Mapped state")
	check(f.width == (0 if detached else (2 if xrgb else 4)) and f.height == (0 if detached else (3 if xrgb else 2)), "Dimensions")
	check(f.stride == f.width * 4 and f.format == "RGBA8", "Output format/stride")
	check(f.transform == transform and f.scale == (2 if scaled else 1), "Scale/transform")
	check(f.logical_width == (f.height if transform & 1 else f.width) / f.scale, "Logical width")
	check(f.logical_height == (f.width if transform & 1 else f.height) / f.scale, "Logical height")
	check(f.pixels == PackedByteArray([] if detached else (XRGB if xrgb else ARGB)), "Exact RGBA bytes/alpha/orientation")
	if not detached:
		check(f.source_stride == (16 if xrgb else 24) and f.source_format == (1 if xrgb else 0), "Source metadata")

func _initialize() -> void:
	call_deferred("run")

func run() -> void:
	var args := OS.get_cmdline_user_args()
	if not check(args.size() == 2, "Expected fixture path and temporary directory"):
		quit(1)
		return
	session = ClassDB.instantiate("MansionCompositorSession")
	for trial in range(8):
		if not check(session.start(args[1]), session.last_error()):
			break
		check(session.snapshot(old_handle).is_empty(), "Stale handle after restart")
		var mode: String = ["frames", "frames", "scale", "transform", "oversize", "format", "divisibility", "truncated"][trial]
		var marker: String = args[1].path_join("frame-%d" % trial)
		pid = OS.create_process(args[0], PackedStringArray([session.socket_path(), marker, mode]))
		if not check(pid > 0, "Spawn fixture"):
			break
		if trial < 2:
			for stage in STAGES:
				if not await wait_marker(marker + "." + stage + ".ready"):
					break
				var handles: PackedInt64Array = session.surface_handles()
				if not check(handles.size() == 1, "One live surface"):
					break
				var f: Dictionary = session.snapshot(handles[0])
				verify(f, stage)
				check(f.handle == handles[0], "Snapshot handle")
				if stage == "initial":
					check(f.handle != old_handle, "Handle changed across restart")
					old_handle = f.handle
					if retained.is_empty():
						retained = f.duplicate(true)
				check(retained.pixels == PackedByteArray(ARGB) and retained.revision == 1, "Old snapshot owns its pixels")
				# Mutating returned data must never mutate the native cached frame.
				var edited: PackedByteArray = f.pixels
				if not edited.is_empty():
					edited[0] = 99
					verify(session.snapshot(handles[0]), stage)
				var file := FileAccess.open(marker + "." + stage + ".go", FileAccess.WRITE)
				if not check(file != null, "Release barrier"):
					break
				file.close()
		if not await wait_marker(marker + ".ok"):
			break
		var deadline := Time.get_ticks_msec() + 5000
		while OS.is_process_running(pid) and Time.get_ticks_msec() < deadline:
			await tick()
		if not check(not OS.is_process_running(pid), "Fixture exit"):
			break
		pid = -1
		for i in range(3):
			await tick()
		check(session.surface_handles().is_empty() and session.snapshot(old_handle).is_empty(), "Destroyed handle invalidated")
		check(retained.pixels == PackedByteArray(ARGB), "Snapshot survives disconnect")
		check(session.stop(), "Stop")
		for stage in STAGES:
			for suffix in [".ready", ".go"]:
				DirAccess.remove_absolute(marker + "." + stage + suffix)
		DirAccess.remove_absolute(marker + ".ok")
	if pid > 0 and OS.is_process_running(pid):
		OS.kill(pid)
	check(session.stop(), "Final stop")
	check(ticks >= 32, "Godot frames advanced")
	print("GODOT_FRAMES_OK barriers=32 negative_cases=6 ticks=", ticks, " failures=", failures)
	quit(1 if failures else 0)
