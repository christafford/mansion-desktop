## Owns the live output session and the one terminal launched by this scene.
extends Node

var session
var child_pid := -1
var handle := 0
var revision := 0
var updates := 0
var texture: ImageTexture
var last_image: Image
var screen: MeshInstance3D
var status: Label3D
var closing := false
var started_ms := 0
const APERTURE := Vector2(0.885, 0.495)

func _ready() -> void:
	if "--no-terminal" in OS.get_cmdline_user_args():
		status.text = "Terminal disabled for scene test"
		set_process(false)
		return
	if not ClassDB.class_exists("MansionCompositorSession"):
		fail("Missing native runtime; run tools/build-godot-runtime.sh")
		return
	session = ClassDB.instantiate("MansionCompositorSession")
	var runtime_dir := OS.get_environment("XDG_RUNTIME_DIR")
	if runtime_dir.is_empty():
		runtime_dir = "/tmp"
	if not session.start(runtime_dir):
		fail(session.last_error())
		return
	var config_dir := ProjectSettings.globalize_path("res://../.tools/terminal-config").simplify_path()
	if DirAccess.make_dir_recursive_absolute(config_dir) != OK:
		fail("Cannot create isolated terminal configuration directory")
		return
	var shell := "/bin/sh"
	if "--terminal-demo" in OS.get_cmdline_user_args():
		shell = ProjectSettings.globalize_path("res://tools/terminal-demo.py")
	var terminal := "/usr/bin/weston-terminal"
	if not FileAccess.file_exists(terminal):
		fail("Weston terminal is unavailable: " + terminal)
		return
	# Environment belongs only to this child; the host Godot connection is intact.
	child_pid = OS.create_process("/usr/bin/env", PackedStringArray([
		"-u", "WAYLAND_SOCKET", "-u", "DISPLAY", "-u", "WAYLAND_DEBUG", "-u", "ENV", "-u", "BASH_ENV", "-u", "PROMPT_COMMAND",
		"WAYLAND_DISPLAY=" + session.socket_path(), "XDG_CONFIG_HOME=" + config_dir,
		"HISTFILE=" + config_dir.path_join("history"),
		"PS1=$ ", "PS2=> ", "INPUTRC=" + ProjectSettings.globalize_path("res://config/terminal.inputrc"),
		terminal, "--font=monospace", "--font-size=16", "--shell=" + shell]))
	if child_pid <= 0:
		fail("Could not launch Weston terminal")
		return
	started_ms = Time.get_ticks_msec()
	status.text = "Starting terminal…"

func fail(message: String) -> void:
	clear_content(message)
	push_error(message)
	set_process(false)

# Snapshots use buffer coordinates. Undo rotation first, then the horizontal
# reflection (Wayland's forward transform is reflection followed by rotation).
static func image_from_frame(frame: Dictionary) -> Image:
	var image := Image.create_from_data(frame.width, frame.height, false, Image.FORMAT_RGBA8, frame.pixels)
	for turn in range(int(frame.transform) % 4):
		image.rotate_90(CLOCKWISE)
	if int(frame.transform) >= 4:
		image.flip_x()
	return image

func show_frame(frame: Dictionary) -> void:
	last_image = image_from_frame(frame)
	if texture == null or Vector2i(texture.get_size()) != last_image.get_size():
		texture = ImageTexture.create_from_image(last_image)
	else:
		texture.update(last_image)
	var mat := screen.material_override as ShaderMaterial
	mat.set_shader_parameter("client_pixels", texture)
	var aspect := float(frame.logical_width) / float(frame.logical_height)
	var size := Vector2(APERTURE.y * aspect, APERTURE.y)
	if size.x > APERTURE.x:
		size = Vector2(APERTURE.x, APERTURE.x / aspect)
	(screen.mesh as QuadMesh).size = size
	screen.visible = true
	status.visible = false
	updates += 1

func clear_content(message: String) -> void:
	screen.visible = false
	(screen.material_override as ShaderMaterial).set_shader_parameter("client_pixels", null)
	texture = null
	last_image = null
	status.text = message
	status.visible = true

func _process(_delta: float) -> void:
	if session == null or closing or not session.is_running():
		return
	if not session.pump():
		fail(session.last_error())
		return
	var windows: PackedInt64Array = session.toplevel_handles()
	if handle != 0 and not windows.has(handle):
		handle = 0
		revision = 0
		clear_content("Terminal closed")
	if handle == 0 and not windows.is_empty():
		handle = windows[0]
		revision = 0
	if handle != 0:
		var frame: Dictionary = session.snapshot(handle, revision)
		if not frame.is_empty():
			revision = frame.revision
			if frame.mapped:
				show_frame(frame)
			else:
				clear_content("Terminal inactive")
	if child_pid > 0 and not child_running():
		if handle == 0:
			clear_content("Terminal closed")
	if updates == 0 and Time.get_ticks_msec() - started_ms > 10000:
		fail("Terminal produced no window content; check the launch log")

func child_running() -> bool:
	if child_pid <= 0:
		return false
	if not OS.is_process_running(child_pid):
		child_pid = -1
		return false
	return true

func shutdown() -> void:
	if closing:
		return
	closing = true
	if session != null:
		if not session.stop():
			push_error(session.last_error())
	# Give the terminal its ordinary display-disconnect cleanup path first.
	var deadline := Time.get_ticks_msec() + 1500
	while child_running() and Time.get_ticks_msec() < deadline:
		await get_tree().process_frame
	if child_running():
		var error := OS.kill(child_pid)
		if error != OK:
			push_error("Could not terminate owned terminal %d: %s" % [child_pid, error_string(error)])
			return # Keep ownership for the final teardown attempt.
	child_pid = -1
	clear_content("Terminal closed")

func _exit_tree() -> void:
	# Covers script errors/forced scene removal in addition to ordinary close.
	if session != null and not session.stop():
		push_error(session.last_error())
	if child_running():
		var error := OS.kill(child_pid)
		if error != OK:
			push_error("Could not terminate owned terminal %d during teardown: %s" % [child_pid, error_string(error)])
