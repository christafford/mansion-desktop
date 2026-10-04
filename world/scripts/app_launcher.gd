## World-mode shell UI. Desktop IDs persist; process/window handles never do.
extends CanvasLayer

const Radial = preload("res://addons/advanced_radial_menu/radial_menu_class.gd")
const TERMINAL_ID := "mansion-weston-terminal.desktop"
var terminal: Node
var app: CanvasLayer
var player: CharacterBody3D
var world_hud: CanvasLayer
var entries: Array = []
var recent: Array[String] = []
var icons: Dictionary = {}
var live_windows: Dictionary = {}
var pending: Dictionary = {}
var catalog_pid := -1
var catalog_due := 0
var initial_recorded := false
var directory := ""
var cache_directory := ""
var helper := ""
var opened := false
var overlay: ColorRect
var wheel: Control
var search_panel: PanelContainer
var query: LineEdit
var results: ItemList
var notice: Label
var open_button: Button
var heading: Label

func _ready() -> void:
	layer = 20
	helper = ProjectSettings.globalize_path("res://tools/desktop-apps.py")
	directory = OS.get_environment("MANSION_LAUNCHER_STATE_DIR")
	if directory.is_empty():
		directory = ProjectSettings.globalize_path("res://../.tools/launcher-state").simplify_path()
	cache_directory = directory.path_join("session-%d-%d" % [OS.get_process_id(), Time.get_ticks_usec()])
	_build_ui()
	if DirAccess.make_dir_recursive_absolute(cache_directory) != OK:
		notice.text = "Cannot create launcher state directory"
		return
	_load_history()
	catalog_pid = OS.create_process("/usr/bin/python3", PackedStringArray([helper, "catalog", "--output", cache_directory.path_join("catalog.json"), "--cache", directory.path_join("icons")]))
	catalog_due = Time.get_ticks_msec() + 15000
	notice.text = "Reading installed applications…" if catalog_pid > 0 else "Could not start application discovery"

func _build_ui() -> void:
	open_button = Button.new()
	open_button.text = "Applications  ·  Tab"
	open_button.position = Vector2(22, 78)
	open_button.add_theme_font_size_override("font_size", 18)
	open_button.pressed.connect(open)
	add_child(open_button)
	overlay = ColorRect.new()
	overlay.color = Color(0.018, 0.028, 0.035, 0.84)
	overlay.gui_input.connect(func(event: InputEvent):
		if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
			close())
	add_child(overlay)
	heading = Label.new()
	heading.text = "Recent applications"
	heading.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	heading.add_theme_font_size_override("font_size", 28)
	heading.mouse_filter = Control.MOUSE_FILTER_IGNORE
	overlay.add_child(heading)
	wheel = Radial.new()
	wheel.first_in_center = true
	wheel.select_action_name = &""
	wheel.keep_selection_outside = false
	wheel.line_rotation_offset_default = -22.5
	wheel.children_size = 96
	wheel.arc_inner_radius = 74
	wheel.color = Color("172c32")
	wheel.hover_color = Color("31565e")
	wheel.line_color = Color("416067")
	wheel.arc_color = Color("86b9bd")
	wheel.arc_line_width = 2
	wheel.arc_detail = 128
	wheel.line_width = 1
	wheel.stroke_enabled = true
	wheel.stroke_width = 2
	wheel.stroke_color = Color("416067")
	wheel.slot_selected.connect(_slot_selected)
	wheel.gui_input.connect(func(event: InputEvent):
		if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
			wheel._process(0.0)
			wheel.select()
			wheel.accept_event())
	overlay.add_child(wheel)
	var center := Label.new()
	center.text = "Find an\napplication\n＋"
	center.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	center.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	center.add_theme_font_size_override("font_size", 17)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	wheel.add_child(center)
	for index in range(8):
		var slot := Control.new()
		slot.mouse_filter = Control.MOUSE_FILTER_IGNORE
		var icon := TextureRect.new()
		icon.name = "Icon"
		icon.position = Vector2(24, 0)
		icon.size = Vector2(48, 48)
		icon.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		icon.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		icon.mouse_filter = Control.MOUSE_FILTER_IGNORE
		slot.add_child(icon)
		var label := Label.new()
		label.name = "Title"
		label.position = Vector2(-20, 52)
		label.size = Vector2(136, 42)
		label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		label.add_theme_font_size_override("font_size", 15)
		label.mouse_filter = Control.MOUSE_FILTER_IGNORE
		slot.add_child(label)
		wheel.add_child(slot)
	search_panel = PanelContainer.new()
	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color("14252d")
	panel_style.border_color = Color("416067")
	panel_style.set_border_width_all(1)
	panel_style.set_corner_radius_all(12)
	search_panel.add_theme_stylebox_override("panel", panel_style)
	overlay.add_child(search_panel)
	var margin := MarginContainer.new()
	for side in ["left", "top", "right", "bottom"]:
		margin.add_theme_constant_override("margin_" + side, 20)
	search_panel.add_child(margin)
	var column := VBoxContainer.new()
	column.add_theme_constant_override("separation", 12)
	margin.add_child(column)
	query = LineEdit.new()
	query.placeholder_text = "Type an application name…"
	query.add_theme_font_size_override("font_size", 22)
	query.text_changed.connect(_filter)
	query.text_submitted.connect(func(_text: String): _launch_selected())
	column.add_child(query)
	results = ItemList.new()
	results.size_flags_vertical = Control.SIZE_EXPAND_FILL
	results.fixed_icon_size = Vector2i(40, 40)
	results.add_theme_font_size_override("font_size", 18)
	results.item_activated.connect(func(_index: int): _launch_selected())
	column.add_child(results)
	var launch_button := Button.new()
	launch_button.text = "Launch selected application"
	launch_button.pressed.connect(_launch_selected)
	column.add_child(launch_button)
	notice = Label.new()
	notice.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	notice.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	notice.mouse_filter = Control.MOUSE_FILTER_IGNORE
	notice.add_theme_font_size_override("font_size", 16)
	overlay.add_child(notice)
	get_viewport().size_changed.connect(_layout)
	_layout()
	_refresh_wheel()
	overlay.hide()
	wheel.enabled = false

func _layout() -> void:
	var viewport := get_viewport().get_visible_rect().size
	overlay.size = viewport
	var diameter := minf(520, minf(viewport.x - 40, viewport.y - 180))
	wheel.size = Vector2.ONE * maxf(300, diameter)
	wheel.position = (viewport - wheel.size) * 0.5
	heading.position = Vector2(20, 28)
	heading.size = Vector2(viewport.x - 40, 44)
	search_panel.size = Vector2(minf(640, viewport.x - 40), maxf(250, viewport.y - 180))
	search_panel.position = (viewport - search_panel.size) * 0.5
	notice.position = Vector2(30, viewport.y - 72)
	notice.size = Vector2(viewport.x - 60, 60)

func entry(identifier: String) -> Dictionary:
	for item in entries:
		if item.id == identifier: return item
	return {}

func _icon(item: Dictionary) -> Texture2D:
	var identifier: String = item.get("id", "")
	if icons.has(identifier): return icons[identifier]
	var path: String = item.get("icon", "")
	if path.is_empty() or not FileAccess.file_exists(path): return null
	var image := Image.load_from_file(path)
	if image == null: return null
	icons[identifier] = ImageTexture.create_from_image(image)
	return icons[identifier]

func _refresh_wheel() -> void:
	for index in range(8):
		var slot := wheel.get_child(index + 1)
		var item := entry(recent[index]) if index < recent.size() else {}
		slot.get_node("Icon").texture = _icon(item)
		slot.get_node("Title").text = str(item.get("name", "—"))
		slot.modulate.a = 1.0 if not item.is_empty() else 0.3
	wheel.force_update()

func open() -> void:
	if app.active or not app.host_focused: return
	opened = true
	app.launcher_active = true
	player.release_pointer()
	player.application_mode = true
	overlay.show()
	wheel.show()
	wheel.enabled = true
	search_panel.hide()
	heading.text = "Recent applications"
	if catalog_pid <= 0 and pending.is_empty() and not entries.is_empty():
		notice.text = "Newest at the top · Clockwise by recency\nClick the center to find an application · Esc to return"
	_refresh_wheel()

func close() -> void:
	opened = false
	app.launcher_active = false
	player.application_mode = app.active
	player.release_pointer()
	wheel.enabled = false
	overlay.hide()
	get_viewport().gui_release_focus()

func show_search() -> void:
	wheel.enabled = false
	wheel.hide()
	search_panel.show()
	heading.text = "Find an application"
	query.text = ""
	_filter("")
	query.grab_focus()

func _filter(text: String) -> void:
	results.clear()
	var terms := text.to_lower().split(" ", false)
	for item in entries:
		var haystack := (str(item.name) + " " + str(item.description) + " " + " ".join(item.keywords)).to_lower()
		var matches := true
		for term in terms:
			if not haystack.contains(term): matches = false
		if not matches: continue
		var index := results.add_item(item.name, _icon(item))
		results.set_item_metadata(index, item.id)
		results.set_item_tooltip(index, item.unavailable if not item.unavailable.is_empty() else item.description)
		results.set_item_disabled(index, not item.unavailable.is_empty())
	for index in range(results.item_count):
		if not results.is_item_disabled(index):
			results.select(index)
			break
	if pending.is_empty():
		notice.text = "No matching applications" if results.item_count == 0 else "Enter or double-click to launch · Esc to return to recents"

func _launch_selected() -> void:
	var selected := results.get_selected_items()
	if not selected.is_empty(): launch(results.get_item_metadata(selected[0]))

func _slot_selected(_slot: Control, index: int) -> void:
	if index == -1: show_search()
	elif index >= 0 and index < recent.size(): launch(recent[index])

func _input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		if not opened and not app.active and event.physical_keycode == KEY_TAB:
			open()
			get_viewport().set_input_as_handled()
		elif opened and event.physical_keycode == KEY_ESCAPE:
			if search_panel.visible: open()
			else: close()
			get_viewport().set_input_as_handled()
		elif opened and search_panel.visible and event.physical_keycode == KEY_DOWN and query.has_focus():
			results.grab_focus()
			get_viewport().set_input_as_handled()
		elif opened and not search_panel.visible:
			if event.physical_keycode >= KEY_1 and event.physical_keycode <= KEY_8:
				_slot_selected(null, event.physical_keycode - KEY_1)
			elif event.physical_keycode == KEY_ENTER: show_search()
			elif event.physical_keycode == KEY_TAB: close()
			get_viewport().set_input_as_handled()

func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT and opened: close()

func _load_history() -> void:
	var path := directory.path_join("recent.json")
	if not FileAccess.file_exists(path): return
	var parsed = JSON.parse_string(FileAccess.get_file_as_string(path))
	if not parsed is Dictionary or parsed.get("version") != 1 or not parsed.get("recent") is Array: return
	for identifier in parsed.recent:
		if identifier is String and not recent.has(identifier) and recent.size() < 8:
			recent.append(identifier)

func remember(identifier: String) -> void:
	recent.erase(identifier)
	recent.push_front(identifier)
	if recent.size() > 8: recent.resize(8)
	var path := directory.path_join("recent.json")
	var file := FileAccess.open(path + ".tmp", FileAccess.WRITE)
	if file == null:
		notice.text = "Application opened, but recent applications could not be saved"
	else:
		file.store_string(JSON.stringify({"version": 1, "recent": recent}))
		file.close()
		if DirAccess.rename_absolute(path + ".tmp", path) != OK:
			notice.text = "Could not save recent applications"
	_refresh_wheel()

func launch(identifier: String) -> void:
	if not pending.is_empty():
		notice.text = "Waiting for the application already starting…"
		return
	var item := entry(identifier)
	if item.is_empty():
		notice.text = "Application is no longer installed"
		return
	if not item.unavailable.is_empty():
		notice.text = item.unavailable
		return
	if terminal.session == null or not terminal.session.is_running():
		notice.text = "The Mansion display is unavailable"
		return
	if live_windows.has(identifier) and terminal.session.toplevel_handles().has(live_windows[identifier]):
		_activate(live_windows[identifier])
		return
	var error_path := cache_directory.path_join("launch-%d.json" % Time.get_ticks_usec())
	var pid := OS.create_process("/usr/bin/python3", PackedStringArray([helper, "launch", "--id", identifier, "--socket", terminal.session.socket_path(), "--output", error_path]))
	if pid <= 0:
		notice.text = "Could not start " + item.name
		return
	terminal.launched_children.append(pid)
	pending = {"id": identifier, "pid": pid, "before": terminal.session.toplevel_handles(), "error": error_path, "deadline": Time.get_ticks_msec() + 12000}
	notice.text = "Starting " + item.name + "…"

func _activate(handle: int) -> void:
	close()
	app.exit_application()
	terminal.select_window(handle)
	app.enter_application()

func _process(_delta: float) -> void:
	open_button.visible = not opened and not app.active
	if world_hud != null: world_hud.visible = not opened and not app.active
	if catalog_pid > 0:
		if not OS.is_process_running(catalog_pid):
			catalog_pid = -1
			var path := cache_directory.path_join("catalog.json")
			var parsed = JSON.parse_string(FileAccess.get_file_as_string(path)) if FileAccess.file_exists(path) else null
			if not parsed is Dictionary or not parsed.get("entries") is Array:
				notice.text = str(parsed.get("error", "Application discovery failed")) if parsed is Dictionary else "Application discovery failed; check Python/GIO/GTK dependencies"
			else:
				entries = parsed.entries
				recent = recent.filter(func(id: String): return not entry(id).is_empty())
				_refresh_wheel()
				if search_panel.visible: _filter(query.text)
				else: notice.text = "Newest at the top · Click the center to find an application"
		elif Time.get_ticks_msec() > catalog_due:
			OS.kill(catalog_pid)
			catalog_pid = -1
			notice.text = "Application discovery timed out"
	if not initial_recorded and terminal.handle != 0 and terminal.screen.visible and not entry(TERMINAL_ID).is_empty():
		initial_recorded = true
		live_windows[TERMINAL_ID] = terminal.handle
		remember(TERMINAL_ID)
	if pending.is_empty(): return
	if FileAccess.file_exists(pending.error):
		var error = JSON.parse_string(FileAccess.get_file_as_string(pending.error))
		notice.text = str(error.get("error", "Launch failed")) if error is Dictionary else "Launch failed"
		pending.clear()
		return
	for handle in terminal.session.toplevel_handles():
		if not pending.before.has(handle):
			var frame: Dictionary = terminal.session.snapshot(handle, 0)
			if frame.get("mapped", false):
				live_windows[pending.id] = handle
				remember(pending.id)
				pending.clear()
				_activate(handle)
				return
	if not terminal.launched_children.has(pending.pid) or Time.get_ticks_msec() > pending.deadline:
		notice.text = "No window appeared. This application may need protocols Mansion does not support yet."
		pending.clear()

func _exit_tree() -> void:
	if catalog_pid > 0 and OS.is_process_running(catalog_pid): OS.kill(catalog_pid)
