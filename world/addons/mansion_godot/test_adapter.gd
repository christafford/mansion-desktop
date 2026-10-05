# Test script to verify GDExtension MansionAdapter is working
extends Node

func _ready() -> void:
	print("Testing GDExtension MansionAdapter...")
	
	# Try to instantiate the GDExtension class directly
	var adapter = ClassDB.instantiate_class("MansionAdapter")
	
	if adapter == null:
		print("ERROR: Failed to create MansionAdapter instance!")
		return
	
	print("SUCCESS: MansionAdapter instance created!")
	
	# Test initialize
	var result = adapter.initialize("mansion-compositor", true)
	print("Initialize result: %s" % result)
	
	if result:
		# Test pump
		var events_processed = adapter.pump()
		print("Pump events processed: %s" % events_processed)
		
		# Test flush
		var flush_result = adapter.flush()
		print("Flush result: %s" % flush_result)
		
		# Test get_focused_serial
		var serial = adapter.get_focused_serial()
		print("Focused serial: %s" % serial)
		
		# Test destroy
		adapter.destroy()
		print("Adapter destroyed")
	else:
		print("Initialize failed!")
	
	# Cleanup
	adapter = null
	print("Test complete!")
