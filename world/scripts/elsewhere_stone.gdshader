shader_type spatial;
uniform vec4 base_color : source_color = vec4(0.25, 0.27, 0.3, 1.0);
uniform bool checker = false;
varying vec3 world_position;
void vertex() { world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz; }
void fragment() {
	vec2 p = world_position.xz;
	vec2 tile = floor(p * 1.6);
	float alternate = mod(tile.x + tile.y, 2.0);
	float vein = sin(p.x * 8.0 + sin(p.y * 5.0) * 2.0 + sin(p.x * 2.0 + p.y * 3.0));
	vec2 edge = abs(fract(p * 1.6) - 0.5);
	float grout = smoothstep(0.485, 0.498, max(edge.x, edge.y));
	vec3 stone = base_color.rgb * (0.95 + 0.05 * vein);
	if (checker) stone *= mix(0.24, 1.0, alternate);
	ALBEDO = mix(stone, base_color.rgb * 0.4, grout);
	ROUGHNESS = 0.62;
}
