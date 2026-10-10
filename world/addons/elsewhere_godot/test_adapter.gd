# Test script to verify GDExtension ElsewhereAdapter is working
extends Node

func _ready() -> void:
	print("Testing GDExtension ElsewhereAdapter...")
	
	# Try to instantiate the GDExtension class directly
	var adapter = ClassDB.instantiate_class("ElsewhereAdapter")
	
	if adapter == null:
		print("ERROR: Failed to create ElsewhereAdapter instance!")
		return
	
	print("SUCCESS: ElsewhereAdapter instance created!")
	
	# Test initialize
	var result = adapter.initialize("elsewhere-compositor", true)
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
