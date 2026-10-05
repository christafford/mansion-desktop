# MansionAdapter - GDScript helper for the GDExtension C library
# Provides a convenient interface for initializing and using the GDExtension
# MansionAdapter class which gives access to the Wayland compositor core.
#
# Usage:
#   var adapter = MansionAdapter.new()
#   adapter.initialize("mansion-compositor")
#   adapter.pump()  # Process events
#   adapter.flush()  # Flush events to clients
#   adapter.destroy()

class_name MansionAdapterHelper extends RefCounted

## Private socket name for the Wayland display
const DEFAULT_SOCKET_NAME = "mansion-compositor"

## The GDExtension adapter instance (created by the C library)
## This will hold the actual GDExtension MansionAdapter instance
var _adapter_instance: RefCounted

## Flag to track if adapter is initialized
var _initialized: bool = false

## Flag to track if adapter is destroyed
var _destroyed: bool = false

## Debug logging
var _debug_logging: bool = false


## Initialize the adapter with optional socket name and debug flag
## Returns true if initialization succeeded
func initialize(socket_name: String = DEFAULT_SOCKET_NAME, debug: bool = false) -> bool:
	if _initialized:
		push_warning("MansionAdapterHelper: Already initialized")
		return true
	
	if _destroyed:
		push_error("MansionAdapterHelper: Cannot initialize after destroy()")
		return false
	
	_debug_logging = debug
	
	# Create an instance of the GDExtension MansionAdapter class
	# The GDExtension class is registered with the name "MansionAdapter"
	# We create it via ClassDB
	_adapter_instance = ClassDB.instantiate_class("MansionAdapter")
	
	if not _adapter_instance:
		push_error("MansionAdapterHelper: Failed to create GDExtension MansionAdapter instance")
		return false
	
	if _debug_logging:
		print("[MansionAdapterHelper] GDExtension MansionAdapter instance created")
	
	# Initialize the adapter via GDExtension
	# The GDExtension MansionAdapter has an initialize() method
	var init_result = _adapter_instance.initialize(socket_name, debug)
	
	if not init_result:
		push_error("MansionAdapterHelper: GDExtension MansionAdapter initialize() failed")
		ClassDB.unref(_adapter_instance)
		_adapter_instance = null
		return false
	
	_initialized = true
	return true


## Shutdown the adapter (calls destroy internally)
func shutdown() -> void:
	destroy()


## Destroy the adapter and free all resources
func destroy() -> void:
	if _destroyed:
		return
	
	if _initialized and _adapter_instance:
		if _debug_logging:
			print("[MansionAdapterHelper] Destroying adapter")
		
		# Call destroy via GDExtension
		_adapter_instance.destroy()
		_adapter_instance = null
	
	_initialized = false
	_destroyed = true


## Pump events from the Wayland event loop (non-blocking)
## Returns the number of events processed, or 0 if not initialized
func pump() -> int:
	if not _initialized or _destroyed or not _adapter_instance:
		return 0
	
	return _adapter_instance.pump()


## Flush pending events to clients
## Returns 0 on success, -1 on error
func flush() -> int:
	if not _initialized or _destroyed or not _adapter_instance:
		return -1
	
	return _adapter_instance.flush()


## Get the current focus serial
## Returns the focus serial number
func get_focused_serial() -> int:
	if not _initialized or _destroyed or not _adapter_instance:
		return 0
	
	return _adapter_instance.get_focused_serial()


## String representation
func _to_string() -> String:
	return "MansionAdapterHelper(initialized=%s, destroyed=%s)" % [_initialized, _destroyed]


## ─── Shm frame snapshot API (P21-T12) ───────────────────────────────────

## Create a snapshot of the committed shm buffer for a surface
## Returns a dictionary with snapshot data or null if no buffer is committed
## Parameters:
##   - client_serial: The client serial ID of the surface
## Returns:
##   - Dictionary with keys: width, height, stride, format, revision, pixels (PoolByteArray), pixels_size
##   - Returns null if no snapshot could be created
func create_snapshot(client_serial: int) -> Dictionary:
	if not _initialized or _destroyed or not _adapter_instance:
		return {}
	
	var snapshot_ptr = _adapter_instance.create_snapshot(client_serial)
	if snapshot_ptr == null:
		return {}
	
	# Extract snapshot data
	var width = _adapter_instance.snapshot_get_width(snapshot_ptr)
	var height = _adapter_instance.snapshot_get_height(snapshot_ptr)
	var stride = _adapter_instance.snapshot_get_stride(snapshot_ptr)
	var format = _adapter_instance.snapshot_get_format(snapshot_ptr)
	var revision = _adapter_instance.snapshot_get_revision(snapshot_ptr)
	var pixels_size = _adapter_instance.snapshot_get_pixels_size(snapshot_ptr)
	var pixels = _adapter_instance.snapshot_get_pixels(snapshot_ptr)
	
	# Convert raw pixel pointer to PoolByteArray
	# Note: This is a simplified conversion; in practice, you may need to handle
	# the pixel data more carefully depending on the GDExtension implementation
	var pixel_array = PoolByteArray()
	if pixels != null and pixels_size > 0:
		# Copy pixel data from the snapshot
		# The actual implementation depends on how GDExtension exposes raw pointers
		pass
	
	# Destroy the snapshot after copying data
	_adapter_instance.snapshot_destroy(snapshot_ptr)
	
	return {
		width = width,
		height = height,
		stride = stride,
		format = format,
		revision = revision,
		pixels_size = pixels_size,
		pixels = pixel_array
	}


## Debug output (if enabled)
func _debug_print(message: String) -> void:
	if _debug_logging:
		print("[MansionAdapterHelper] %s" % message)
