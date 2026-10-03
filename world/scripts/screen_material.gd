## Preserve client sRGB under the study's fixed Compatibility/Filmic environment.
## The inverse lookup is evaluated once, not as a per-pixel iterative shader.
extends RefCounted

static func displayed_channel(albedo: float) -> float:
	# Matches the pinned engine's GLES3 tonemap_inc.glsl, exposure=white=1.
	var linear := albedo * (albedo * (albedo * 0.305306011 + 0.682171111) + 0.012522878)
	var mapped := ((linear * (0.88 * linear + 0.06) + 0.002) / (linear * (0.88 * linear + 0.6) + 0.06)) - 0.01 / 0.3
	var white := ((0.88 + 0.06 + 0.002) / (0.88 + 0.6 + 0.06)) - 0.01 / 0.3
	return maxf(1.055 * pow(maxf(mapped / white, 0.0), 1.0 / 2.4) - 0.055, 0.0)

static func create() -> ShaderMaterial:
	var lookup := Image.create(256, 1, false, Image.FORMAT_RF)
	for value in range(256):
		var low := 0.0
		var high := 1.0
		for step in range(24):
			var mid := (low + high) * 0.5
			if displayed_channel(mid) < float(value) / 255.0:
				low = mid
			else:
				high = mid
		lookup.set_pixel(value, 0, Color((low + high) * 0.5, 0, 0))
	var material := ShaderMaterial.new()
	material.shader = preload("res://scripts/application_screen.gdshader")
	material.set_shader_parameter("inverse_tonemap", ImageTexture.create_from_image(lookup))
	return material
